/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "BuildEnvironment/Script/IEnvironmentScript.hpp"

#include "Terminal/Files.hpp"

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

}
