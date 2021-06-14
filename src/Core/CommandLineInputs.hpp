/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_COMMAND_LINE_INPUTS_HPP
#define CHALET_COMMAND_LINE_INPUTS_HPP

#include "Compile/ToolchainPreference.hpp"
#include "Generator/IdeType.hpp"
#include "Router/Route.hpp"

namespace chalet
{
struct CommandLineInputs
{
	CommandLineInputs();

	const std::string& defaultEnvFile() const noexcept;

	const std::string& buildFile() const noexcept;
	void setBuildFile(std::string&& inValue) noexcept;

	const std::string& buildPath() const noexcept;
	void setBuildPath(std::string&& inValue) noexcept;

	Route command() const noexcept;
	void setCommand(const Route inValue) noexcept;

	const std::string& buildConfiguration() const noexcept;
	void setBuildConfiguration(const std::string& inValue) noexcept;

	const std::string& buildFromCommandLine() const noexcept;
	void setBuildFromCommandLine(std::string&& inValue) noexcept;

	const std::string& platform() const noexcept;
	const StringList& notPlatforms() const noexcept;

	const std::string& runProject() const noexcept;
	void setRunProject(std::string&& inValue) noexcept;

	const StringList& runOptions() const noexcept;
	void setRunOptions(std::string&& inValue) noexcept;

	const std::string& appPath() const noexcept;
	void setAppPath(const std::string& inValue) noexcept;

	IdeType generator() const noexcept;
	const std::string& generatorRaw() const noexcept;
	void setGenerator(std::string&& inValue) noexcept;

	const ToolchainPreference& toolchainPreference() const noexcept;
	const std::string& toolchainPreferenceRaw() const noexcept;
	void setToolchainPreference(std::string&& inValue) const noexcept;

	const std::string& initProjectName() const noexcept;
	void setInitProjectName(std::string&& inValue) noexcept;

	const std::string& initPath() const noexcept;
	void setInitPath(std::string&& inValue) noexcept;

	const std::string& envFile() const noexcept;
	void setEnvFile(std::string&& inValue) noexcept;

	const std::string& archRaw() const noexcept;
	void setArchRaw(const std::string& inValue) noexcept;

	const std::string& hostArchitecture() const noexcept;
	const std::string& targetArchitecture() const noexcept;
	void setTargetArchitecture(std::string&& inValue) noexcept;

	bool saveSchemaToFile() const noexcept;
	void setSaveSchemaToFile(const bool inValue) noexcept;

private:
	std::string getPlatform() const noexcept;
	StringList getNotPlatforms() const noexcept;

	ToolchainPreference getToolchainPreferenceFromString(const std::string& inValue) const;
	IdeType getIdeTypeFromString(const std::string& inValue) const;

	mutable ToolchainPreference m_toolchainPreference;

	StringList m_runOptions;
	StringList m_notPlatforms;

	const std::string kDefaultEnvFile;
	std::string m_buildFile;
	std::string m_buildPath;
	std::string m_buildConfiguration;
	std::string m_buildFromCommandLine;
	std::string m_platform;
	std::string m_runProject;
	std::string m_appPath;
	std::string m_generatorRaw;
	mutable std::string m_toolchainPreferenceRaw;

	std::string m_initProjectName;
	std::string m_initPath;
	std::string m_envFile;
	std::string m_archRaw;
	std::string m_hostArchitecture;
	std::string m_targetArchitecture;

	Route m_command = Route::Unknown;
	IdeType m_generator = IdeType::None;

	mutable bool m_buildPathMade = false;
	bool m_saveSchemaToFile = false;
};
}

#endif // CHALET_COMMAND_LINE_INPUTS_HPP
