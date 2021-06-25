/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_COMMAND_LINE_INPUTS_HPP
#define CHALET_COMMAND_LINE_INPUTS_HPP

#include "Compile/ToolchainPreference.hpp"
#include "Generator/IdeType.hpp"
#include "Router/Route.hpp"
#include "Settings/SettingsType.hpp"

namespace chalet
{
struct CommandLineInputs
{
	CommandLineInputs();

	void detectToolchainPreference() const;

	const std::string& workingDirectory() const noexcept;
	const std::string& homeDirectory() const noexcept;
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

	SettingsType settingsType() const noexcept;
	void setSettingsType(const SettingsType inValue) noexcept;

	const std::string& settingsKey() const noexcept;
	void setSettingsKey(std::string&& inValue) noexcept;

	const std::string& settingsValue() const noexcept;
	void setSettingsValue(std::string&& inValue) noexcept;

private:
	std::string getPlatform() const noexcept;
	StringList getNotPlatforms() const noexcept;

	ToolchainPreference getToolchainPreferenceFromString(const std::string& inValue) const;
	IdeType getIdeTypeFromString(const std::string& inValue) const;

	void clearWorkingDirectory(std::string& outValue);

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
	std::string m_settingsKey;
	std::string m_settingsValue;

	mutable std::string m_toolchainPreferenceRaw;
	mutable std::string m_workingDirectory;
	mutable std::string m_homeDirectory;

	std::string m_initPath;
	std::string m_envFile;
	std::string m_archRaw;
	std::string m_hostArchitecture;
	std::string m_targetArchitecture;

	Route m_command = Route::Unknown;
	IdeType m_generator = IdeType::None;
	SettingsType m_settingsType = SettingsType::None;

	bool m_saveSchemaToFile = false;
};
}

#endif // CHALET_COMMAND_LINE_INPUTS_HPP
