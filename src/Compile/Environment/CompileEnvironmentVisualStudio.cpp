/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/Environment/CompileEnvironmentVisualStudio.hpp"

#include "Cache/SourceCache.hpp"
#include "Cache/WorkspaceCache.hpp"
#include "Core/Arch.hpp"
#include "Core/CommandLineInputs.hpp"
#include "State/BuildInfo.hpp"
#include "State/BuildState.hpp"
#include "State/CompilerTools.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Environment.hpp"
#include "Terminal/Output.hpp"
#include "Terminal/Unicode.hpp"
#include "Utility/RegexPatterns.hpp"
#include "Utility/String.hpp"
#include "Utility/Timer.hpp"

namespace chalet
{
static struct
{
	short exists = -1;
	std::string vswhere;
} state;

/*****************************************************************************/
bool CompileEnvironmentVisualStudio::exists()
{
#if defined(CHALET_WIN32)
	if (state.exists == -1)
	{
		// TODO:
		//   Note that if you install vswhere using Chocolatey (instead of the VS/MSBuild installer),
		//   it will be located at %ProgramData%\chocolatey\lib\vswhere\tools\vswhere.exe
		//   https://stackoverflow.com/questions/54305638/how-to-find-vswhere-exe-path

		std::string progFiles = Environment::getAsString("ProgramFiles(x86)");
		state.vswhere = fmt::format("{}\\Microsoft Visual Studio\\Installer\\vswhere.exe", progFiles);

		bool vswhereFound = Commands::pathExists(state.vswhere);
		if (!vswhereFound)
		{
			std::string progFiles64 = Environment::getAsString("ProgramFiles");
			String::replaceAll(state.vswhere, progFiles, progFiles64);

			vswhereFound = Commands::pathExists(state.vswhere);
			if (!vswhereFound)
			{
				// Do this one last to try to support legacy (< VS 2017)
				state.vswhere = Commands::which("vswhere");
				vswhereFound = !state.vswhere.empty();
			}
		}

		state.exists = vswhereFound ? 1 : 0;
	}

	return state.exists == 1;
#else
	return false;
#endif
}

/*****************************************************************************/
CompileEnvironmentVisualStudio::CompileEnvironmentVisualStudio(const ToolchainType inType, const CommandLineInputs& inInputs, BuildState& inState) :
	ICompileEnvironment(inType, inInputs, inState)
{
}

/*****************************************************************************/
std::string CompileEnvironmentVisualStudio::getIdentifier() const noexcept
{
	return std::string("msvc");
}

/*****************************************************************************/
bool CompileEnvironmentVisualStudio::validateArchitectureFromInput()
{
	if (m_msvcArchitectureSet)
		return true;

	auto gnuArchToMsvcArch = [](const std::string& inArch) -> std::string {
		if (String::equals("x86_64", inArch))
			return "x64";
		else if (String::equals("i686", inArch))
			return "x86";
		else if (String::equals("aarch64", inArch))
			return "arm64";

		return inArch;
	};

	auto splitHostTarget = [](std::string& outHost, std::string& outTarget) -> void {
		if (!String::contains('_', outTarget))
			return;

		auto split = String::split(outTarget, '_');

		if (outHost.empty())
			outHost = split.front();

		outTarget = split.back();
	};

	std::string host;
	std::string target = gnuArchToMsvcArch(m_inputs.targetArchitecture());

	const auto& compiler = m_state.toolchain.compilerCxxAny().path;
	if (!compiler.empty())
	{
		std::string lower = String::toLowerCase(compiler);
		auto search = lower.find("/bin/host");
		if (search == std::string::npos)
		{
			Diagnostic::error("MSVC Host architecture was not detected in compiler path: {}", compiler);
			return false;
		}

		auto nextPath = lower.find('/', search + 5);
		if (search == std::string::npos)
		{
			Diagnostic::error("MSVC Host architecture was not detected in compiler path: {}", compiler);
			return false;
		}

		search += 9;
		std::string hostFromCompilerPath = lower.substr(search, nextPath - search);
		search = nextPath + 1;
		nextPath = lower.find('/', search);
		if (search == std::string::npos)
		{
			Diagnostic::error("MSVC Target architecture was not detected in compiler path: {}", compiler);
			return false;
		}

		splitHostTarget(host, target);
		if (host.empty())
			host = hostFromCompilerPath;

		std::string targetFromCompilerPath = lower.substr(search, nextPath - search);
		if (target.empty() || (target == targetFromCompilerPath && host == hostFromCompilerPath))
		{
			target = lower.substr(search, nextPath - search);
		}
		else
		{
			const auto& preferenceName = m_inputs.toolchainPreferenceName();
			Diagnostic::error("Expected host '{}' and target '{}'. Please use a different toolchain or create a new one for this architecture.", hostFromCompilerPath, targetFromCompilerPath);
			Diagnostic::error("Architecture '{}' is not supported by the '{}' toolchain.", m_inputs.targetArchitecture(), preferenceName);
			return false;
		}
	}
	else
	{
		if (target.empty())
			target = gnuArchToMsvcArch(m_inputs.hostArchitecture());

		splitHostTarget(host, target);

		if (host.empty())
			host = gnuArchToMsvcArch(m_inputs.hostArchitecture());
	}

	m_state.info.setHostArchitecture(host);

	if (host == target)
		m_config.varsAllArch = target;
	else
		m_config.varsAllArch = fmt::format("{}_{}", host, target);

	m_inputs.setTargetArchitecture(m_config.varsAllArch);
	m_state.info.setTargetArchitecture(fmt::format("{}-pc-windows-msvc", Arch::toGnuArch(target)));

	// TODO: universal windows platform - uwp-windows-msvc

	m_msvcArchitectureSet = true;

	return true;
}

/*****************************************************************************/
// Other environments (Intel) might want to inherit the MSVC environment, so we make some of these function static
//
bool CompileEnvironmentVisualStudio::makeEnvironment(VisualStudioEnvironmentConfig& outConfig, const std::string& inVersion, const BuildState& inState)
{
	outConfig.pathVariable = Environment::getPath();

	// Note: See Note about __CHALET_MSVC_INJECT__ in Environment.cpp
	auto appDataPath = Environment::getAsString("APPDATA");
	outConfig.pathInject = fmt::format("{}\\__CHALET_MSVC_INJECT__", appDataPath);

	// we got here from a preset in the command line
	outConfig.isPreset = outConfig.inVersion != VisualStudioVersion::None;

	auto getStartOfVsWhereCommand = [&]() {
		StringList cmd{ state.vswhere, "-nologo" };
		const auto vsVersion = outConfig.inVersion;
		const bool isStable = vsVersion == VisualStudioVersion::Stable;
		const bool isPreview = vsVersion == VisualStudioVersion::Preview;

		if (!isStable)
			cmd.emplace_back("-prerelease");

		if (isStable || isPreview)
		{
			cmd.emplace_back("-latest");
		}
		else
		{
			ushort ver = static_cast<ushort>(vsVersion);
			ushort next = ver + 1;
			cmd.emplace_back("-version");
			cmd.emplace_back(fmt::format("[{},{})", ver, next));
		}

		return cmd;
	};

	auto getProductsOptions = [](StringList& outCmd) {
		outCmd.emplace_back("-products");
		outCmd.emplace_back("Microsoft.VisualStudio.Product.Enterprise");
		outCmd.emplace_back("Microsoft.VisualStudio.Product.Professional");
		outCmd.emplace_back("Microsoft.VisualStudio.Product.Community");
	};

	auto getMsvcVersion = [&getStartOfVsWhereCommand, &getProductsOptions]() {
		StringList vswhereCmd = getStartOfVsWhereCommand();
		getProductsOptions(vswhereCmd);
		vswhereCmd.emplace_back("-property");
		vswhereCmd.emplace_back("installationVersion");
		return Commands::subprocessOutput(vswhereCmd);
	};

	bool deltaExists = Commands::pathExists(outConfig.varsFileMsvcDelta);
	if (!deltaExists)
	{
		Diagnostic::infoEllipsis("Creating Microsoft{} Visual C/C++ Environment Cache", Unicode::registered());

		auto getFirstVisualStudioPathFromVsWhere = [](const StringList& inCmd) {
			auto temp = Commands::subprocessOutput(inCmd);
			auto split = String::split(temp, "\n");
			return split.front();
		};

		if (outConfig.isPreset)
		{
			StringList vswhereCmd = getStartOfVsWhereCommand();
			getProductsOptions(vswhereCmd);
			vswhereCmd.emplace_back("-property");
			vswhereCmd.emplace_back("installationPath");

			outConfig.visualStudioPath = getFirstVisualStudioPathFromVsWhere(vswhereCmd);

			outConfig.detectedVersion = getMsvcVersion();
		}
		else if (RegexPatterns::matchesFullVersionString(inVersion))
		{
			StringList vswhereCmd{ state.vswhere, "-nologo" };
			vswhereCmd.emplace_back("-prerelease"); // always include prereleases in this scenario since we're search for the exact version
			vswhereCmd.emplace_back("-version");
			vswhereCmd.push_back(inVersion);
			getProductsOptions(vswhereCmd);
			vswhereCmd.emplace_back("-property");
			vswhereCmd.emplace_back("installationPath");

			outConfig.visualStudioPath = getFirstVisualStudioPathFromVsWhere(vswhereCmd);
			if (String::startsWith("Error", outConfig.visualStudioPath))
				outConfig.visualStudioPath.clear();

			outConfig.detectedVersion = inVersion;
		}
		else
		{
			Diagnostic::error("Toolchain version string '{}' is invalid. For MSVC, this must be the full installation version", inVersion);
			return false;
		}

		if (outConfig.visualStudioPath.empty())
		{
			Diagnostic::error("MSVC Environment could not be fetched: vswhere could not find a matching Visual Studio installation.");
			return false;
		}

		if (!Commands::pathExists(outConfig.visualStudioPath))
		{
			Diagnostic::error("MSVC Environment could not be fetched: The path to Visual Studio could not be found. ({})", outConfig.visualStudioPath);
			return false;
		}

		// Read the current environment and save it to a file
		if (!ICompileEnvironment::saveOriginalEnvironment(outConfig.varsFileOriginal, inState))
		{
			Diagnostic::error("MSVC Environment could not be fetched: The original environment could not be saved.");
			return false;
		}

		// Read the MSVC environment and save it to a file
		if (!CompileEnvironmentVisualStudio::saveMsvcEnvironment(outConfig))
		{
			Diagnostic::error("MSVC Environment could not be fetched: The expected method returned with error.");
			return false;
		}

		// Get the delta between the two and save it to a file
		ICompileEnvironment::createEnvironmentDelta(outConfig.varsFileOriginal, outConfig.varsFileMsvc, outConfig.varsFileMsvcDelta, [&](std::string& line) {
			if (String::startsWith("__VSCMD_PREINIT_PATH=", line))
			{
				if (String::contains(outConfig.pathInject, line))
					String::replaceAll(line, outConfig.pathInject + ";", "");
			}
			else if (String::startsWith({ "PATH=", "Path=" }, line))
			{
				String::replaceAll(line, outConfig.pathVariable, "");
			}
			String::replaceAll(line, "\\\\", "\\");
		});
	}
	else
	{
		Diagnostic::infoEllipsis("Reading Microsoft{} Visual C/C++ Environment Cache", Unicode::registered());

		if (outConfig.isPreset)
			outConfig.detectedVersion = getMsvcVersion();
	}

	return true;
}

/*****************************************************************************/
void CompileEnvironmentVisualStudio::populateVariables(VisualStudioEnvironmentConfig& outConfig, Dictionary<std::string>& outVariables)
{
	const auto pathKey = Environment::getPathKey();
	for (auto& [name, var] : outVariables)
	{
		if (String::equals(pathKey, name))
		{
			if (String::contains(outConfig.pathInject, outConfig.pathVariable))
			{
				String::replaceAll(outConfig.pathVariable, outConfig.pathInject, var);
				Environment::set(name.c_str(), outConfig.pathVariable);
			}
			else
			{
				Environment::set(name.c_str(), outConfig.pathVariable + ";" + var);
			}
		}
		else
		{
			Environment::set(name.c_str(), var);
		}
	}

	if (outConfig.visualStudioPath.empty())
	{
		auto vsappDir = outVariables.find("VSINSTALLDIR");
		if (vsappDir != outVariables.end())
		{
			outConfig.visualStudioPath = vsappDir->second;
		}
	}
}

/*****************************************************************************/
bool CompileEnvironmentVisualStudio::createFromVersion(const std::string& inVersion)
{
	// This sets vswhere
	if (!CompileEnvironmentVisualStudio::exists())
		return true;

	Timer timer;

	std::string uid;
	if (inVersion.empty())
	{
		uid = std::to_string(static_cast<std::underlying_type_t<VisualStudioVersion>>(m_inputs.visualStudioVersion()));
	}
	else
	{
		auto firstPeriod = inVersion.find_first_not_of("0123456789");
		if (firstPeriod != std::string::npos)
			uid = inVersion.substr(0, firstPeriod);
		else
			uid = inVersion;
	}

	m_config.varsFileOriginal = m_state.cache.getHashPath(fmt::format("{}_original.env", this->identifier()), CacheType::Local);
	m_config.varsFileMsvc = m_state.cache.getHashPath(fmt::format("{}_all.env", this->identifier()), CacheType::Local);
	m_config.varsFileMsvcDelta = getVarsPath(uid);
	m_config.varsAllArchOptions = m_inputs.archOptions();
	m_config.inVersion = m_inputs.visualStudioVersion();

	m_ouptuttedDescription = true;

	if (!CompileEnvironmentVisualStudio::makeEnvironment(m_config, inVersion, m_state))
		return false;

	m_detectedVersion = std::move(m_config.detectedVersion);

	// Read delta to cache
	Dictionary<std::string> variables;
	ICompileEnvironment::cacheEnvironmentDelta(m_config.varsFileMsvcDelta, variables);

	CompileEnvironmentVisualStudio::populateVariables(m_config, variables);

	if (m_config.isPreset)
		m_inputs.setToolchainPreferenceName(makeToolchainName());

	m_state.cache.file().addExtraHash(String::getPathFilename(m_config.varsFileMsvcDelta));

	m_config.pathVariable.clear();

	Diagnostic::printDone(timer.asString());

	return true;
}

/*****************************************************************************/
std::string CompileEnvironmentVisualStudio::makeToolchainName() const
{
	std::string ret;
	if (!m_detectedVersion.empty())
	{
		auto versionSplit = String::split(m_detectedVersion, '.');
		if (versionSplit.size() >= 1)
		{
			chalet_assert(!m_config.varsAllArch.empty(), "vcVarsAll arch was not set");

			ret = fmt::format("{}{}{}", m_config.varsAllArch, m_state.info.targetArchitectureTripleSuffix(), versionSplit.front());
		}
	}
	return ret;
}

/*****************************************************************************/
StringList CompileEnvironmentVisualStudio::getVersionCommand(const std::string& inExecutable) const
{
	return { inExecutable };
}

/*****************************************************************************/
std::string CompileEnvironmentVisualStudio::getFullCxxCompilerString(const std::string& inVersion) const
{
	const auto& vsVersion = m_detectedVersion.empty() ? m_state.toolchain.version() : m_detectedVersion;
	return fmt::format("Microsoft{} Visual C/C++ version {} (VS {})", Unicode::registered(), inVersion, vsVersion);
}

/*****************************************************************************/
bool CompileEnvironmentVisualStudio::getCompilerVersionAndDescription(CompilerInfo& outInfo) const
{
	Timer timer;

	auto& sourceCache = m_state.cache.file().sources();
	std::string cachedVersion;
	if (sourceCache.versionRequriesUpdate(outInfo.path, cachedVersion))
	{
		// Microsoft (R) C/C++ Optimizing Compiler Version 19.28.29914 for x64
		std::string rawOutput = Commands::subprocessOutput(getVersionCommand(outInfo.path));

		auto splitOutput = String::split(rawOutput, '\n');
		if (splitOutput.size() >= 2)
		{
			auto start = splitOutput[1].find("Version");
			auto end = splitOutput[1].find(" for ");
			if (start != std::string::npos && end != std::string::npos)
			{
				start += 8;
				auto version = splitOutput[1].substr(start, end - start); // cl.exe version
				version = version.substr(0, version.find_first_not_of("0123456789."));
				cachedVersion = std::move(version);

				// const auto arch = splitOutput[1].substr(end + 5);
			}
		}
	}

	if (!cachedVersion.empty())
	{
		outInfo.version = std::move(cachedVersion);

		sourceCache.addVersion(outInfo.path, outInfo.version);

		outInfo.description = getFullCxxCompilerString(outInfo.version);
		return true;
	}
	else
	{
		outInfo.description = "Unrecognized";
		return false;
	}
}

/*****************************************************************************/
std::vector<CompilerPathStructure> CompileEnvironmentVisualStudio::getValidCompilerPaths() const
{
	std::vector<CompilerPathStructure> ret;

#if defined(CHALET_WIN32)
	ret.push_back({ "/bin/hostx64/x64", "/lib/x64", "/include" });
	ret.push_back({ "/bin/hostx64/x86", "/lib/x86", "/include" });
	ret.push_back({ "/bin/hostx64/arm64", "/lib/arm64", "/include" });
	ret.push_back({ "/bin/hostx64/arm", "/lib/arm", "/include" });

	ret.push_back({ "/bin/hostx86/x86", "/lib/x86", "/include" });
	ret.push_back({ "/bin/hostx86/x64", "/lib/x64", "/include" });
	ret.push_back({ "/bin/hostx86/arm64", "/lib/arm64", "/include" });
	ret.push_back({ "/bin/hostx86/arm", "/lib/arm", "/include" });

	// ret.push_back({"/bin/hostx64/x64", "/lib/64", "/include"});
#endif

	return ret;
}

/*****************************************************************************/
bool CompileEnvironmentVisualStudio::verifyToolchain()
{
	return true;
}

/*****************************************************************************/
bool CompileEnvironmentVisualStudio::compilerVersionIsToolchainVersion() const
{
	return false;
}

/*****************************************************************************/
bool CompileEnvironmentVisualStudio::saveMsvcEnvironment(VisualStudioEnvironmentConfig& outConfig)
{
	std::string vcvarsFile{ "vcvarsall" };
	StringList allowedArchesWin = CompileEnvironmentVisualStudio::getAllowedArchitectures();

	if (!String::equals(allowedArchesWin, outConfig.varsAllArch))
	{
		Diagnostic::error("Requested arch '{}' is not supported by {}.bat", outConfig.varsAllArch, vcvarsFile);
		return false;
	}

	// https://docs.microsoft.com/en-us/cpp/build/building-on-the-command-line?view=msvc-160
	auto vcVarsAll = fmt::format("\"{}\\VC\\Auxiliary\\Build\\{}.bat\"", outConfig.visualStudioPath, vcvarsFile);
	StringList cmd{ vcVarsAll, outConfig.varsAllArch };

	for (auto& arg : outConfig.varsAllArchOptions)
	{
		cmd.push_back(arg);
	}

	cmd.emplace_back(">");
	cmd.emplace_back("nul");
	cmd.emplace_back("&&");
	cmd.emplace_back("SET");
	cmd.emplace_back(">");
	cmd.push_back(outConfig.varsFileMsvc);

	bool result = std::system(String::join(cmd).c_str()) == EXIT_SUCCESS;
	return result;
}

/*****************************************************************************/
StringList CompileEnvironmentVisualStudio::getAllowedArchitectures()
{
	// clang-format off
	return {
		"x86",								// any host, x86 target
		"x86_x64", /*"x86_amd64",*/			// any host, x64 target
		"x86_arm",							// any host, ARM target
		"x86_arm64",						// any host, ARM64 target
		//
		"x64", /*"amd64",*/					// x64 host, x64 target
		"x64_x86", /*"amd64_x86",*/			// x64 host, x86 target
		"x64_arm", /*"amd64_arm",*/			// x64 host, ARM target
		"x64_arm64", /*"amd64_arm64",*/		// x64 host, ARMG64 target
	};
	// clang-format on
}
}
