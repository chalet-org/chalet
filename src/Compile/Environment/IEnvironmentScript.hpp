/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_IENVIRONMENT_SCRIPT_HPP
#define CHALET_IENVIRONMENT_SCRIPT_HPP

namespace chalet
{
class BuildState;
struct IEnvironmentScript
{
	virtual ~IEnvironmentScript() = default;

	virtual bool makeEnvironment(const BuildState& inState) = 0;
	virtual void readEnvironmentVariablesFromDeltaFile() = 0;

	virtual void setEnvVarsFileBefore(const std::string& inValue) final;
	virtual void setEnvVarsFileAfter(const std::string& inValue) final;

	virtual const std::string& envVarsFileDelta() const noexcept final;
	virtual void setEnvVarsFileDelta(const std::string& inValue) final;
	virtual bool envVarsFileDeltaExists() const noexcept final;

protected:
	virtual bool saveEnvironmentFromScript() = 0;
	virtual StringList getAllowedArchitectures() = 0;

	std::string m_envVarsFileBefore;
	std::string m_envVarsFileAfter;
	std::string m_envVarsFileDelta;

	std::string m_pathVariable;

	bool m_envVarsFileDeltaExists = false;
};
}

#endif // CHALET_IENVIRONMENT_SCRIPT_HPP
