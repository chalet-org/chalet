/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/Environment/CompileEnvironmentVisualStudio.hpp"

#include "Core/Arch.hpp"
#include "Core/CommandLineInputs.hpp"
#include "State/BuildState.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Environment.hpp"
#include "Terminal/Output.hpp"
#include "Terminal/Unicode.hpp"
#include "Utility/RegexPatterns.hpp"
#include "Utility/String.hpp"
#include "Utility/Timer.hpp"

namespace chalet
{
#if defined(CHALET_WIN32)
static struct
{
	short exists = -1;
	std::string vswhere;
} state;
#endif

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
CompileEnvironmentVisualStudio::CompileEnvironmentVisualStudio(const CommandLineInputs& inInputs, BuildState& inState) :
	ICompileEnvironment(inInputs, inState),
	kVarsId("msvc")
{
	UNUSED(kVarsId);
}

/*****************************************************************************/
ToolchainType CompileEnvironmentVisualStudio::type() const noexcept
{
	return ToolchainType::VisualStudio;
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
bool CompileEnvironmentVisualStudio::createFromVersion(const std::string& inVersion)
{
#if defined(CHALET_WIN32)
	m_varsFileOriginal = m_state.cache.getHashPath(fmt::format("{}_original.env", kVarsId), CacheType::Local);
	m_varsFileMsvc = m_state.cache.getHashPath(fmt::format("{}_all.env", kVarsId), CacheType::Local);
	m_varsFileMsvcDelta = getVarsPath(kVarsId);

	// This sets state.vswhere
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

	auto getMsvcVersion = [this, &getStartOfVsWhereCommand]() {
		StringList vswhereCmd = getStartOfVsWhereCommand();
		vswhereCmd.emplace_back("-property");
		vswhereCmd.emplace_back("installationVersion");
		return Commands::subprocessOutput(vswhereCmd);
	};

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
			vswhereCmd.push_back(fmt::format("{}", inVersion));
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
		m_varsFileMsvcDelta = getVarsPath(kVarsId);
		if (m_varsFileMsvcDelta != old)
		{
			Commands::copyRename(old, m_varsFileMsvcDelta, true);
		}
	}

	m_state.cache.file().addExtraHash(String::getPathFilename(m_varsFileMsvcDelta));

	Diagnostic::printDone(timer.asString());
#else
	UNUSED(inVersion);
#endif
	return true;
}

/*****************************************************************************/
bool CompileEnvironmentVisualStudio::saveMsvcEnvironment() const
{
#if defined(CHALET_WIN32)
	std::string vcvarsFile{ "vcvarsall" };
	StringList allowedArchesWin = Arch::getAllowedMsvcArchitectures();

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
#else
	return false;
#endif
}

/*****************************************************************************/
bool CompileEnvironmentVisualStudio::validateArchitectureFromInput()
{
#if defined(CHALET_WIN32)
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

	const auto& compiler = m_state.toolchain.compilerCxx();
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
#endif

	return true;
}

}
