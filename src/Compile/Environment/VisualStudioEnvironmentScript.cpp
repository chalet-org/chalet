/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/Environment/VisualStudioEnvironmentScript.hpp"

#include "State/AncillaryTools.hpp"
#include "State/BuildState.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Environment.hpp"
#include "Utility/RegexPatterns.hpp"
#include "Utility/String.hpp"

namespace chalet
{
namespace
{
static struct
{
	short exists = -1;
	std::string vswhere;
} state;
}

/*****************************************************************************/
bool VisualStudioEnvironmentScript::visualStudioExists()
{
#if defined(CHALET_WIN32)
	if (state.exists == -1)
	{
		std::string progFiles = Environment::getAsString("ProgramFiles(x86)");
		state.vswhere = fmt::format("{}\\Microsoft Visual Studio\\Installer\\vswhere.exe", progFiles);

		bool vswhereFound = Commands::pathExists(state.vswhere);
		if (!vswhereFound)
		{
			std::string progData = Environment::getAsString("ProgramData");
			state.vswhere = fmt::format("{}\\chocolatey\\lib\\vswhere\\tools\\vswhere.exe", progData);
			vswhereFound = Commands::pathExists(state.vswhere);
		}
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
	auto appDataPath = Environment::getAsString("APPDATA");
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
		Environment::createDeltaEnvFile(m_envVarsFileBefore, m_envVarsFileAfter, m_envVarsFileDelta, [&](std::string& line) {
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
		ushort ver = static_cast<ushort>(inVersion);
		ushort next = ver + 1;
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
	cmd.emplace_back("nul");
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
