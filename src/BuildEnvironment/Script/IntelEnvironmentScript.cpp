/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "BuildEnvironment/Script/IntelEnvironmentScript.hpp"

#include "Core/CommandLineInputs.hpp"
#include "Process/Environment.hpp"
#include "State/BuildInfo.hpp"
#include "State/BuildState.hpp"
#include "System/Files.hpp"
#include "Terminal/Shell.hpp"
#include "Utility/Path.hpp"
#include "Utility/String.hpp"

namespace chalet
{
IntelEnvironmentScript::IntelEnvironmentScript(const CommandLineInputs& inInputs) :
	m_inputs(inInputs)
{
}

/*****************************************************************************/
bool IntelEnvironmentScript::isPreset() noexcept
{
	return m_inputs.isToolchainPreset();
}

/*****************************************************************************/
bool IntelEnvironmentScript::makeEnvironment(const BuildState& inState)
{
	m_pathVariable = Environment::getPath();
	m_intelSetVarsArch = inState.info.targetArchitectureString();
	m_vsVersion = m_inputs.getVisualStudioYear();

	if (!m_envVarsFileDeltaExists)
	{
#if defined(CHALET_WIN32)
		auto oneApiRoot = Environment::getString("ONEAPI_ROOT");
		if (oneApiRoot.back() == '/' || oneApiRoot.back() == '\\')
			oneApiRoot.pop_back();

		m_intelSetVars = fmt::format("{}/setvars.bat", oneApiRoot);
		Path::toUnix(m_intelSetVars);
#else
		const auto& home = m_inputs.homeDirectory();
		m_intelSetVars = fmt::format("{}/intel/oneapi/setvars.sh", home);
#endif
		if (!Files::pathExists(m_intelSetVars))
		{
#if !defined(CHALET_WIN32)
			m_intelSetVars = "/opt/intel/oneapi/setvars.sh";
			if (!Files::pathExists(m_intelSetVars))
#endif
			{
				Diagnostic::error("No suitable Intel C++ compiler installation found. Pleas install the Intel oneAPI Toolkit before continuing.");
				return false;
			}
		}

		// Read the current environment and save it to a file
		if (!Environment::saveToEnvFile(m_envVarsFileBefore))
		{
			Diagnostic::error("Intel Environment could not be fetched: The original environment could not be saved.");
			return false;
		}

		if (!saveEnvironmentFromScript())
		{
			Diagnostic::error("Intel Environment could not be fetched: The expected method returned with error.");
			return false;
		}

		Environment::createDeltaEnvFile(m_envVarsFileBefore, m_envVarsFileAfter, m_envVarsFileDelta, [this](std::string& line) {
			if (String::startsWith("PATH=", line) || String::startsWith("Path=", line))
			{
				String::replaceAll(line, m_pathVariable, "");
			}
		});
	}

	return true;
}

/*****************************************************************************/
bool IntelEnvironmentScript::saveEnvironmentFromScript()
{
#if defined(CHALET_WIN32)
	StringList cmd{ fmt::format("\"{}\"", m_intelSetVars) };

	StringList allowedArches = getAllowedArchitectures();
	if (!String::equals(allowedArches, m_intelSetVarsArch))
	{
		auto setVarsFile = String::getPathFilename(m_intelSetVars);
		Diagnostic::error("Requested arch '{}' is not supported by {}", m_inputs.getResolvedTargetArchitecture(), setVarsFile);
		return false;
	}

	std::string arch;
	if (String::equals("i686", m_intelSetVarsArch))
		arch = "ia32";
	else
		arch = "intel64";

	cmd.emplace_back(std::move(arch));

	if (m_vsVersion >= 2017)
	{
		cmd.emplace_back(fmt::format("vs{}", m_vsVersion));
	}

	cmd.emplace_back(">");
	cmd.emplace_back(Shell::getNull());
	cmd.emplace_back("&&");
	cmd.emplace_back("SET");
	cmd.emplace_back(">");
	cmd.push_back(m_envVarsFileAfter);
#else
	StringList shellCmd;
	shellCmd.emplace_back("source");
	shellCmd.push_back(m_intelSetVars);
	shellCmd.emplace_back("--force");
	shellCmd.emplace_back(">");
	shellCmd.emplace_back(Shell::getNull());
	shellCmd.emplace_back("&&");
	shellCmd.emplace_back("printenv");
	shellCmd.emplace_back(">");
	shellCmd.push_back(m_envVarsFileAfter);
	auto shellCmdString = String::join(shellCmd);

	auto bash = Environment::getShell();

	StringList cmd;
	cmd.emplace_back(std::move(bash));
	cmd.emplace_back("-c");
	cmd.emplace_back(fmt::format("'{}'", shellCmdString));

#endif
	auto outCmd = String::join(cmd);
	bool result = std::system(outCmd.c_str()) == EXIT_SUCCESS;

	return result;
}

/*****************************************************************************/
StringList IntelEnvironmentScript::getAllowedArchitectures()
{
	return {
		"x86_64",
		"i686",
	};
}

/*****************************************************************************/
std::string IntelEnvironmentScript::getPathVariable(const std::string& inNewPath) const
{
	constexpr char pathSep = Environment::getPathSeparator();
	auto path = IEnvironmentScript::getPathVariable(inNewPath);

	auto clangPath = Environment::getString("ONEAPI_ROOT");
	if (clangPath.empty())
		return path;

	Path::toWindows(clangPath);
	if (clangPath.back() == '\\')
		clangPath.pop_back();

	clangPath = fmt::format("{}\\compiler\\latest\\windows\\bin-llvm", clangPath);

	return fmt::format("{}{}{}", clangPath, pathSep, path);
}
}
