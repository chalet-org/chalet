/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "BuildEnvironment/Script/IEnvironmentScript.hpp"

#include "Process/Environment.hpp"
#include "System/Files.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
void IEnvironmentScript::setEnvVarsFileBefore(const std::string& inValue)
{
	m_envVarsFileBefore = inValue;
}

/*****************************************************************************/
void IEnvironmentScript::setEnvVarsFileAfter(const std::string& inValue)
{
	m_envVarsFileAfter = inValue;
}

/*****************************************************************************/
const std::string& IEnvironmentScript::envVarsFileDelta() const noexcept
{
	return m_envVarsFileDelta;
}

/*****************************************************************************/
void IEnvironmentScript::setEnvVarsFileDelta(const std::string& inValue)
{
	if (inValue.empty())
		return;

	m_envVarsFileDelta = inValue;

	m_envVarsFileDeltaExists = Files::pathExists(m_envVarsFileDelta);
}

/*****************************************************************************/
bool IEnvironmentScript::envVarsFileDeltaExists() const noexcept
{
	return m_envVarsFileDeltaExists;
}

/*****************************************************************************/
Dictionary<std::string> IEnvironmentScript::readEnvironmentVariablesFromDeltaFile()
{
	Dictionary<std::string> variables;

	Environment::readEnvFileToDictionary(m_envVarsFileDelta, variables);

#if !defined(CHALET_WIN32)
	const auto pathKey = Environment::getPathKey();
#endif
	for (auto& [name, var] : variables)
	{
#if defined(CHALET_WIN32)
		if (String::equals("path", String::toLowerCase(name)))
#else
		if (String::equals(pathKey, name))
#endif
		{
			Environment::set(name.c_str(), getPathVariable(var));
		}
		else
		{
			Environment::set(name.c_str(), var);
		}
	}

	return variables;
}

/*****************************************************************************/
std::string IEnvironmentScript::getPathVariable(const std::string& inNewPath) const
{
	constexpr char pathSep = Environment::getPathSeparator();
	return fmt::format("{}{}{}", inNewPath, pathSep, m_pathVariable);
}
}
