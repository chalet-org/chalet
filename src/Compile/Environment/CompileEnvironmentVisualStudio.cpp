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
bool CompileEnvironmentVisualStudio::createFromVersion(const std::string& inVersion)
{
	m_varsFileOriginal = m_state.cache.getHashPath(fmt::format("{}_original.env", this->identifier()), CacheType::Local);
	m_varsFileMsvc = m_state.cache.getHashPath(fmt::format("{}_all.env", this->identifier()), CacheType::Local);
	m_varsFileMsvcDelta = getVarsPath(this->identifier());

	// This sets vswhere
	if (!CompileEnvironmentVisualStudio::exists())
		return true;

	Timer timer;

	// TODO: Check if Visual Studio is even installed

	m_path = Environment::getPath();

	// Note: See Note about __CHALET_MSVC_INJECT__ in Environment.cpp
	auto appDataPath = Environment::getAsString("APPDATA");
	std::string msvcInject = fmt::format("{}\\__CHALET_MSVC_INJECT__", appDataPath);

	std::string installationVersion;

	// we got here from a preset in the command line
	bool genericMsvcFromInput = m_inputs.visualStudioVersion() != VisualStudioVersion::None;

	auto getStartOfVsWhereCommand = [this]() {
		StringList cmd{ state.vswhere, "-nologo" };
		const auto vsVersion = m_inputs.visualStudioVersion();
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

	m_ouptuttedDescription = true;

	bool deltaExists = Commands::pathExists(m_varsFileMsvcDelta);
	if (!deltaExists)
	{
		Diagnostic::infoEllipsis("Creating Microsoft{} Visual C/C++ Environment Cache", Unicode::registered());

		auto getFirstVisualStudioPathFromVsWhere = [](const StringList& inCmd) {
			auto temp = Commands::subprocessOutput(inCmd);
			auto split = String::split(temp, "\n");
			return split.front();
		};

		if (genericMsvcFromInput)
		{
			StringList vswhereCmd = getStartOfVsWhereCommand();
			getProductsOptions(vswhereCmd);
			vswhereCmd.emplace_back("-property");
			vswhereCmd.emplace_back("installationPath");
			m_vsAppIdDir = getFirstVisualStudioPathFromVsWhere(vswhereCmd);

			m_detectedVersion = getMsvcVersion();
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

			m_vsAppIdDir = getFirstVisualStudioPathFromVsWhere(vswhereCmd);
			if (String::startsWith("Error", m_vsAppIdDir))
				m_vsAppIdDir.clear();

			m_detectedVersion = inVersion;
		}
		else
		{
			Diagnostic::error("Toolchain version string '{}' is invalid. For MSVC, this must be the full installation version", inVersion);
			return false;
		}

		if (m_vsAppIdDir.empty())
		{
			Diagnostic::error("MSVC Environment could not be fetched: vswhere could not find a matching Visual Studio installation.");
			return false;
		}

		if (!Commands::pathExists(m_vsAppIdDir))
		{
			Diagnostic::error("MSVC Environment could not be fetched: The path to Visual Studio could not be found. ({})", m_vsAppIdDir);
			return false;
		}

		// Read the current environment and save it to a file
		if (!saveOriginalEnvironment(m_varsFileOriginal))
		{
			Diagnostic::error("MSVC Environment could not be fetched: The original environment could not be saved.");
			return false;
		}

		// Read the MSVC environment and save it to a file
		if (!saveMsvcEnvironment())
		{
			Diagnostic::error("MSVC Environment could not be fetched: The expected method returned with error.");
			return false;
		}

		// Get the delta between the two and save it to a file
		createEnvironmentDelta(m_varsFileOriginal, m_varsFileMsvc, m_varsFileMsvcDelta, [&](std::string& line) {
			if (String::startsWith("__VSCMD_PREINIT_PATH=", line))
			{
				if (String::contains(msvcInject, line))
					String::replaceAll(line, msvcInject + ";", "");
			}
			else if (String::startsWith({ "PATH=", "Path=" }, line))
			{
				String::replaceAll(line, m_path, "");
			}
			String::replaceAll(line, "\\\\", "\\");
		});
	}
	else
	{
		Diagnostic::infoEllipsis("Reading Microsoft{} Visual C/C++ Environment Cache", Unicode::registered());

		if (genericMsvcFromInput)
			m_detectedVersion = getMsvcVersion();
	}

	// Read delta to cache
	cacheEnvironmentDelta(m_varsFileMsvcDelta);

	StringList pathSearch{ "Path", "PATH" };
	for (auto& [name, var] : m_variables)
	{
		if (String::equals(pathSearch, name))
		{
			if (String::contains(msvcInject, m_path))
			{
				String::replaceAll(m_path, msvcInject, var);
				Environment::set(name.c_str(), m_path);
			}
			else
			{
				Environment::set(name.c_str(), m_path + ";" + var);
			}
		}
		else
		{
			Environment::set(name.c_str(), var);
		}
	}

	if (m_vsAppIdDir.empty())
	{
		auto vsappDir = m_variables.find("VSINSTALLDIR");
		if (vsappDir != m_variables.end())
		{
			m_vsAppIdDir = vsappDir->second;
		}
	}

	if (genericMsvcFromInput)
	{
		if (!m_detectedVersion.empty())
		{
			auto versionSplit = String::split(m_detectedVersion, '.');
			if (versionSplit.size() >= 1)
			{
				std::string name = fmt::format("{}-pc-windows-msvc{}", m_inputs.targetArchitecture(), versionSplit.front());
				m_inputs.setToolchainPreferenceName(std::move(name));
			}
		}

		auto old = m_varsFileMsvcDelta;
		m_varsFileMsvcDelta = getVarsPath(this->identifier());
		if (m_varsFileMsvcDelta != old)
		{
			Commands::copyRename(old, m_varsFileMsvcDelta, true);
		}
	}

	m_state.cache.file().addExtraHash(String::getPathFilename(m_varsFileMsvcDelta));

	Diagnostic::printDone(timer.asString());

	return true;
}

/*****************************************************************************/
bool CompileEnvironmentVisualStudio::validateArchitectureFromInput()
{
	if (m_msvcArchitectureSet)
		return true;

	auto normalizeArch = [](const std::string& inArch) -> std::string {
		if (String::equals("x86_64", inArch))
			return "x64";
		else if (String::equals("i686", inArch))
			return "x86";

		return inArch;
	};

	std::string host;
	std::string target;

	const auto& compiler = m_state.toolchain.compilerCxxAny().path;
	if (!compiler.empty())
	{
		// old: detectTargetArchitectureMSVC()

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
		host = lower.substr(search, nextPath - search);
		search = nextPath + 1;
		nextPath = lower.find('/', search);
		if (search == std::string::npos)
		{
			Diagnostic::error("MSVC Target architecture was not detected in compiler path: {}", compiler);
			return false;
		}

		target = lower.substr(search, nextPath - search);
	}
	else
	{
		target = m_inputs.targetArchitecture();
		if (target.empty())
		{
			// Try to get the architecture from the name
			const auto& preferenceName = m_inputs.toolchainPreferenceName();
			auto regexResult = RegexPatterns::matchesTargetArchitectureWithResult(preferenceName);
			if (!regexResult.empty())
			{
				target = regexResult;
			}
			else
			{
				target = normalizeArch(m_inputs.hostArchitecture());
			}
		}

		if (String::contains('_', target))
		{
			auto split = String::split(target, '_');
			if (String::equals("64", split.back()))
			{
				target = "x64";
			}
			else
			{
				host = split.front();
				target = split.back();
			}
		}

		if (host.empty())
			host = normalizeArch(m_inputs.hostArchitecture());
	}

	m_state.info.setHostArchitecture(host);

	if (host == target)
		m_inputs.setTargetArchitecture(target);
	else
		m_inputs.setTargetArchitecture(fmt::format("{}_{}", host, target));

	m_state.info.setTargetArchitecture(m_inputs.targetArchitecture());

	m_msvcArchitectureSet = true;

	return true;
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

		outInfo.arch = m_state.info.targetArchitectureTriple();
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
bool CompileEnvironmentVisualStudio::saveMsvcEnvironment() const
{
	std::string vcvarsFile{ "vcvarsall" };
	StringList allowedArchesWin = getAllowedArchitectures();

	const auto& targetArch = m_inputs.targetArchitecture();
	if (!String::equals(allowedArchesWin, targetArch))
	{
		Diagnostic::error("Requested arch '{}' is not supported by {}.bat", targetArch, vcvarsFile);
		return false;
	}

	// https://docs.microsoft.com/en-us/cpp/build/building-on-the-command-line?view=msvc-160
	auto vcVarsAll = fmt::format("\"{}\\VC\\Auxiliary\\Build\\{}.bat\"", m_vsAppIdDir, vcvarsFile);
	StringList cmd{ vcVarsAll, targetArch };

	for (auto& arg : m_inputs.archOptions())
	{
		cmd.push_back(arg);
	}

	cmd.emplace_back(">");
	cmd.emplace_back("nul");
	cmd.emplace_back("&&");
	cmd.emplace_back("SET");
	cmd.emplace_back(">");
	cmd.push_back(m_varsFileMsvc);

	bool result = std::system(String::join(cmd).c_str()) == EXIT_SUCCESS;
	return result;
}

/*****************************************************************************/
StringList CompileEnvironmentVisualStudio::getAllowedArchitectures() const
{
	// clang-format off
	return {
		"x86",						// any host, x86 target
		"x86_x64", "x86_amd64",		// any host, x64 target
		"x86_arm",					// any host, ARM target
		"x86_arm64",				// any host, ARM64 target
		//
		"x64", "amd64",				// x64 host, x64 target
		"x64_x86", "amd64_x86",		// x64 host, x86 target
		"x64_arm", "amd64_arm",		// x64 host, ARM target
		"x64_arm64", "amd64_arm64",	// x64 host, ARMG64 target
	};
	// clang-format on
}
}
