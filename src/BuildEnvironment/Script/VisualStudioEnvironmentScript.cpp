/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "BuildEnvironment/Script/VisualStudioEnvironmentScript.hpp"

#include "Core/CommandLineInputs.hpp"
#include "Platform/Arch.hpp"
#include "Process/Environment.hpp"
#include "State/AncillaryTools.hpp"
#include "State/BuildState.hpp"
#include "State/CompilerTools.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Shell.hpp"
#include "Utility/RegexPatterns.hpp"
#include "Utility/String.hpp"

namespace chalet
{
namespace
{
static struct
{
	i16 exists = -1;
	std::string vswhere;
} state;
}

/*****************************************************************************/
bool VisualStudioEnvironmentScript::visualStudioExists()
{
#if defined(CHALET_WIN32)
	if (state.exists == -1)
	{
		std::string progFiles = Environment::getString("ProgramFiles(x86)");
		state.vswhere = fmt::format("{}\\Microsoft Visual Studio\\Installer\\vswhere.exe", progFiles);

		bool vswhereFound = Commands::pathExists(state.vswhere);
		if (!vswhereFound)
		{
			std::string progData = Environment::getString("ProgramData");
			state.vswhere = fmt::format("{}\\chocolatey\\lib\\vswhere\\tools\\vswhere.exe", progData);
			vswhereFound = Commands::pathExists(state.vswhere);
		}
		if (!vswhereFound)
		{
			std::string progFiles64 = Environment::getString("ProgramFiles");
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
const std::string& VisualStudioEnvironmentScript::architecture() const noexcept
{
	return m_varsAllArch;
}

/*****************************************************************************/
void VisualStudioEnvironmentScript::setArchitecture(const std::string& inHost, const std::string& inTarget, const StringList& inOptions)
{
	if (inHost == inTarget)
		m_varsAllArch = inTarget;
	else
		m_varsAllArch = fmt::format("{}_{}", inHost, inTarget);

	m_varsAllArchOptions = inOptions;
}

/*****************************************************************************/
void VisualStudioEnvironmentScript::setVersion(const std::string& inValue, const VisualStudioVersion inVsVersion)
{
	m_rawVersion = inValue;
	m_vsVersion = inVsVersion;

	if (m_rawVersion.empty())
	{
		m_detectedVersion = getVisualStudioVersion(m_vsVersion);

		// If there is more than one version installed, prefer the first version retrieved
		auto lineBreak = m_detectedVersion.find('\n');
		if (lineBreak != std::string::npos)
		{
			m_detectedVersion = m_detectedVersion.substr(0, lineBreak);
		}
	}
	else
	{
		m_detectedVersion = m_rawVersion;
	}
}

/*****************************************************************************/
const std::string& VisualStudioEnvironmentScript::detectedVersion() const noexcept
{
	return m_detectedVersion;
}

/*****************************************************************************/
bool VisualStudioEnvironmentScript::isPreset() noexcept
{
	return m_vsVersion != VisualStudioVersion::None;
}

/*****************************************************************************/
// Other environments (Intel) might want to inherit the MSVC environment, so we make some of these function static
//
bool VisualStudioEnvironmentScript::makeEnvironment(const BuildState& inState)
{
	UNUSED(inState);
	m_pathVariable = Environment::getPath();

	// Note: See Note about __CHALET_PATH_INJECT__ in Environment.cpp
	auto appDataPath = Environment::getString("APPDATA");
	m_pathInject = fmt::format("{}\\__CHALET_PATH_INJECT__", appDataPath);

	if (!m_envVarsFileDeltaExists)
	{
		auto getFirstVisualStudioPathFromVsWhere = [](const StringList& inCmd) {
			auto temp = Commands::subprocessOutput(inCmd);
			auto split = String::split(temp, "\n");
			return split.front();
		};

		if (isPreset())
		{
			StringList vswhereCmd = getStartOfVsWhereCommand(m_vsVersion);
			addProductOptions(vswhereCmd);
			vswhereCmd.emplace_back("-property");
			vswhereCmd.emplace_back("installationPath");

			m_visualStudioPath = getFirstVisualStudioPathFromVsWhere(vswhereCmd);

			if (m_detectedVersion.empty())
				m_detectedVersion = getVisualStudioVersion(m_vsVersion);
		}
		else if (RegexPatterns::matchesFullVersionString(m_rawVersion))
		{
			StringList vswhereCmd{ state.vswhere, "-nologo" };
			vswhereCmd.emplace_back("-prerelease"); // always include prereleases in this scenario since we're search for the exact version
			vswhereCmd.emplace_back("-version");
			vswhereCmd.push_back(m_rawVersion);
			addProductOptions(vswhereCmd);
			vswhereCmd.emplace_back("-property");
			vswhereCmd.emplace_back("installationPath");

			m_visualStudioPath = getFirstVisualStudioPathFromVsWhere(vswhereCmd);
			if (String::startsWith("Error", m_visualStudioPath))
				m_visualStudioPath.clear();

			m_detectedVersion = m_rawVersion;
		}
		else
		{
			Diagnostic::error("Toolchain version string '{}' is invalid. For MSVC, this must be the full installation version", m_rawVersion);
			return false;
		}

		if (m_detectedVersion.empty())
		{
			Diagnostic::error("MSVC Environment could not be fetched: vswhere could not find a matching Visual Studio installation.");
			return false;
		}

		if (!Commands::pathExists(m_visualStudioPath))
		{
			Diagnostic::error("MSVC Environment could not be fetched: The path to Visual Studio could not be found. ({})", m_visualStudioPath);
			return false;
		}

		// Read the current environment and save it to a file
		if (!Environment::saveToEnvFile(m_envVarsFileBefore))
		{
			Diagnostic::error("MSVC Environment could not be fetched: The original environment could not be saved.");
			return false;
		}

		// Read the MSVC environment and save it to a file
		if (!saveEnvironmentFromScript())
		{
			Diagnostic::error("MSVC Environment could not be fetched: The expected method returned with error.");
			return false;
		}

		// Get the delta between the two and save it to a file
		Environment::createDeltaEnvFile(m_envVarsFileBefore, m_envVarsFileAfter, m_envVarsFileDelta, [this](std::string& line) {
			if (String::startsWith("__VSCMD_PREINIT_PATH=", line))
			{
				if (String::contains(m_pathInject, line))
					String::replaceAll(line, m_pathInject + Environment::getPathSeparator(), "");
			}
			else if (String::startsWith(StringList{ "PATH=", "Path=" }, line))
			{
				String::replaceAll(line, m_pathVariable, "");
			}
			String::replaceAll(line, "\\\\", "\\");
		});
	}
	else
	{
		if (isPreset() && m_detectedVersion.empty())
			m_detectedVersion = getVisualStudioVersion(m_vsVersion);
	}

	return true;
}

/*****************************************************************************/
void VisualStudioEnvironmentScript::readEnvironmentVariablesFromDeltaFile()
{
	Dictionary<std::string> variables;
	Environment::readEnvFileToDictionary(m_envVarsFileDelta, variables);

#if !defined(CHALET_WIN32)
	const auto pathKey = Environment::getPathKey();
#endif
	const char pathSep = Environment::getPathSeparator();
	for (auto& [name, var] : variables)
	{
#if defined(CHALET_WIN32)
		if (String::equals("path", String::toLowerCase(name)))
#else
		if (String::equals(pathKey, name))
#endif
		{
			if (String::contains(m_pathInject, m_pathVariable))
			{
				String::replaceAll(m_pathVariable, m_pathInject, var);
				Environment::set(name.c_str(), m_pathVariable);
			}
			else
			{
				Environment::set(name.c_str(), fmt::format("{}{}{}", m_pathVariable, pathSep, var));
			}
		}
		else
		{
			Environment::set(name.c_str(), var);
		}
	}

	if (m_visualStudioPath.empty())
	{
		auto vsappDir = variables.find("VSINSTALLDIR");
		if (vsappDir != variables.end())
		{
			m_visualStudioPath = vsappDir->second;
		}
	}
}

/*****************************************************************************/
StringList VisualStudioEnvironmentScript::getStartOfVsWhereCommand(const VisualStudioVersion inVersion)
{
	StringList cmd{ state.vswhere, "-nologo" };
	const bool isStable = inVersion == VisualStudioVersion::Stable;
	const bool isPreview = inVersion == VisualStudioVersion::Preview;

	if (!isStable)
		cmd.emplace_back("-prerelease");

	if (isStable || isPreview)
	{
		cmd.emplace_back("-latest");
	}
	else
	{
		u16 ver = static_cast<u16>(inVersion);
		u16 next = ver + 1;
		cmd.emplace_back("-version");
		cmd.emplace_back(fmt::format("[{},{})", ver, next));
	}

	return cmd;
}

/*****************************************************************************/
void VisualStudioEnvironmentScript::addProductOptions(StringList& outCmd)
{
	outCmd.emplace_back("-products");
	outCmd.emplace_back("Microsoft.VisualStudio.Product.Enterprise");
	outCmd.emplace_back("Microsoft.VisualStudio.Product.Professional");
	outCmd.emplace_back("Microsoft.VisualStudio.Product.Community");
}

/*****************************************************************************/
std::string VisualStudioEnvironmentScript::getVisualStudioVersion(const VisualStudioVersion inVersion)
{
	StringList vswhereCmd = getStartOfVsWhereCommand(inVersion);
	addProductOptions(vswhereCmd);
	vswhereCmd.emplace_back("-property");
	vswhereCmd.emplace_back("installationVersion");
	return Commands::subprocessOutput(vswhereCmd);
}

/*****************************************************************************/
bool VisualStudioEnvironmentScript::saveEnvironmentFromScript()
{
	std::string vcvarsFile{ "vcvarsall" };
	StringList allowedArchesWin = getAllowedArchitectures();

	if (!String::equals(allowedArchesWin, m_varsAllArch))
	{
		Diagnostic::error("Requested arch '{}' is not supported by {}.bat", m_varsAllArch, vcvarsFile);
		return false;
	}

	// https://docs.microsoft.com/en-us/cpp/build/building-on-the-command-line?view=msvc-160
	auto vcVarsAll = fmt::format("\"{}\\VC\\Auxiliary\\Build\\{}.bat\"", m_visualStudioPath, vcvarsFile);
	StringList cmd{ vcVarsAll, m_varsAllArch };

	for (auto& arg : m_varsAllArchOptions)
	{
		cmd.push_back(arg);
	}

	cmd.emplace_back(">");
	cmd.emplace_back(Shell::getNull());
	cmd.emplace_back("&&");
	cmd.emplace_back("SET");
	cmd.emplace_back(">");
	cmd.push_back(m_envVarsFileAfter);

	bool result = std::system(String::join(cmd).c_str()) == EXIT_SUCCESS;
	return result;
}

/*****************************************************************************/
StringList VisualStudioEnvironmentScript::getAllowedArchitectures()
{
	StringList ret{
		// clang-format off
		"x86",								// any host, x86 target
		"x86_x64", /*"x86_amd64",*/			// any host, x64 target
		"x86_arm",							// any host, ARM target
		"x86_arm64",						// any host, ARM64 target
		//
		"x64", /*"amd64",*/					// x64 host, x64 target
		"x64_x86", /*"amd64_x86",*/			// x64 host, x86 target
		"x64_arm", /*"amd64_arm",*/			// x64 host, ARM target
		"x64_arm64", /*"amd64_arm64",*/		// x64 host, ARM64 target
		// clang-format on
	};

	auto arch = Arch::getHostCpuArchitecture();
	if (String::equals("arm64", arch))
	{
		// Note: these are untested
		//   https://devblogs.microsoft.com/visualstudio/arm64-visual-studio
		//
		// clang-format off
		ret.emplace_back("arm64");			// ARM64 host, ARM64 target
		ret.emplace_back("arm64_x64");		// ARM64 host, x64 target
		ret.emplace_back("arm64_x86");		// ARM64 host, x86 target
		// clang-format on
	}

	return ret;
}

/*****************************************************************************/
bool VisualStudioEnvironmentScript::validateArchitectureFromInput(const BuildState& inState, std::string& outHost, std::string& outTarget)
{
	auto gnuArchToMsvcArch = [](const std::string& inArch) -> std::string {
		if (String::equals("x86_64", inArch))
			return "x64";
		else if (String::equals("i686", inArch))
			return "x86";
		else if (String::equals("aarch64", inArch))
			return "arm64";

		return inArch;
	};

	auto splitHostTarget = [](std::string& outHost2, std::string& outTarget2) -> void {
		if (!String::contains('_', outTarget2))
			return;

		auto split = String::split(outTarget2, '_');

		if (outHost2.empty())
			outHost2 = split.front();

		outTarget2 = split.back();
	};

	std::string host;
	std::string target = gnuArchToMsvcArch(inState.inputs.getResolvedTargetArchitecture());

	const auto& compiler = inState.toolchain.compilerCxxAny().path;
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
			const auto& preferenceName = inState.inputs.toolchainPreferenceName();
			Diagnostic::error("Expected host '{}' and target '{}'. Please use a different toolchain or create a new one for this architecture.", hostFromCompilerPath, targetFromCompilerPath);
			Diagnostic::error("Architecture '{}' is not supported by the '{}' toolchain.", inState.inputs.getResolvedTargetArchitecture(), preferenceName);
			return false;
		}
	}
	else
	{
		if (target.empty())
			target = gnuArchToMsvcArch(inState.inputs.hostArchitecture());

		splitHostTarget(host, target);

		if (host.empty())
			host = gnuArchToMsvcArch(inState.inputs.hostArchitecture());
	}

	setArchitecture(host, target, inState.inputs.archOptions());

	outHost = std::move(host);
	outTarget = std::move(target);

	return true;
}
}
