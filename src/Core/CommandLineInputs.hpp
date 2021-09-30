/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_COMMAND_LINE_INPUTS_HPP
#define CHALET_COMMAND_LINE_INPUTS_HPP

#include <optional>

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

	const std::string& defaultInputFile() const noexcept;
	const std::string& defaultSettingsFile() const noexcept;
	const std::string& defaultEnvFile() const noexcept;
	const std::string& defaultOutputDirectory() const noexcept;
	const std::string& defaultExternalDirectory() const noexcept;
	const std::string& defaultDistributionDirectory() const noexcept;

	const std::string& inputFile() const noexcept;
	void setInputFile(std::string&& inValue) noexcept;

	const std::string& settingsFile() const noexcept;
	void setSettingsFile(std::string&& inValue) noexcept;
	const std::string& globalSettingsFile() const noexcept;

	const std::string& rootDirectory() const noexcept;
	void setRootDirectory(std::string&& inValue) noexcept;

	const std::string& outputDirectory() const noexcept;
	void setOutputDirectory(std::string&& inValue) noexcept;

	const std::string& externalDirectory() const noexcept;
	void setExternalDirectory(std::string&& inValue) noexcept;

	const std::string& distributionDirectory() const noexcept;
	void setDistributionDirectory(std::string&& inValue) noexcept;

	Route command() const noexcept;
	void setCommand(const Route inValue) noexcept;

	const std::string& buildConfiguration() const noexcept;
	void setBuildConfiguration(std::string&& inValue) noexcept;

	const std::string& platform() const noexcept;
	const StringList& notPlatforms() const noexcept;

	const std::string& runTarget() const noexcept;
	void setRunTarget(std::string&& inValue) noexcept;

	const StringList& runOptions() const noexcept;
	void setRunOptions(std::string&& inValue) noexcept;

	const std::string& appPath() const noexcept;
	void setAppPath(const std::string& inValue) noexcept;

	IdeType generator() const noexcept;
	const std::string& generatorRaw() const noexcept;
	void setGenerator(std::string&& inValue) noexcept;

	const ToolchainPreference& toolchainPreference() const noexcept;
	void setToolchainPreference(std::string&& inValue) const noexcept;
	const std::string& toolchainPreferenceName() const noexcept;
	void setToolchainPreferenceName(std::string&& inValue) const noexcept;
	bool isMsvcPreRelease() const noexcept;
	bool isToolchainPreset() const noexcept;

	const std::string& initPath() const noexcept;
	void setInitPath(std::string&& inValue) noexcept;

	const std::string& envFile() const noexcept;
	void setEnvFile(std::string&& inValue) noexcept;

	const std::string& architectureRaw() const noexcept;
	void setArchitectureRaw(std::string&& inValue) const noexcept;

	const std::string& hostArchitecture() const noexcept;
	const std::string& targetArchitecture() const noexcept;
	void setTargetArchitecture(const std::string& inValue) const noexcept;

	const StringList& universalArches() const noexcept;

	const StringList& archOptions() const noexcept;
	void setArchOptions(StringList&& inList) const noexcept;
	std::string getArchWithOptionsAsString(const std::string& inArchBase) const;

	bool saveSchemaToFile() const noexcept;
	void setSaveSchemaToFile(const bool inValue) noexcept;

	SettingsType settingsType() const noexcept;
	void setSettingsType(const SettingsType inValue) noexcept;

	const std::string& settingsKey() const noexcept;
	void setSettingsKey(std::string&& inValue) noexcept;

	const std::string& settingsValue() const noexcept;
	void setSettingsValue(std::string&& inValue) noexcept;

	void clearWorkingDirectory(std::string& outValue) const;

	const std::optional<uint>& maxJobs() const noexcept;
	void setMaxJobs(const uint inValue) noexcept;

	const std::optional<bool>& dumpAssembly() const noexcept;
	void setDumpAssembly(const bool inValue) noexcept;

	const std::optional<bool>& showCommands() const noexcept;
	void setShowCommands(const bool inValue) noexcept;

	const std::optional<bool>& benchmark() const noexcept;
	void setBenchmark(const bool inValue) noexcept;

	const std::optional<bool>& generateCompileCommands() const noexcept;
	void setGenerateCompileCommands(const bool inValue) noexcept;

private:
	std::string getPlatform() const noexcept;
	StringList getNotPlatforms() const noexcept;

	ToolchainPreference getToolchainPreferenceFromString(const std::string& inValue) const;
	IdeType getIdeTypeFromString(const std::string& inValue) const;

	mutable ToolchainPreference m_toolchainPreference;

	StringList m_runOptions;
	StringList m_notPlatforms;
	mutable StringList m_universalArches;
	mutable StringList m_archOptions;

	const std::string kDefaultInputFile;
	const std::string kDefaultSettingsFile;
	const std::string kDefaultEnvFile;
	const std::string kDefaultOutputDirectory;
	const std::string kDefaultExternalDirectory;
	const std::string kDefaultDistributionDirectory;

	std::string m_inputFile;
	std::string m_settingsFile;
	std::string m_rootDirectory;
	std::string m_outputDirectory;
	std::string m_externalDirectory;
	std::string m_distributionDirectory;
	std::string m_buildConfiguration;
	std::string m_buildFromCommandLine;
	std::string m_platform;
	std::string m_runTarget;
	std::string m_appPath;
	std::string m_generatorRaw;
	std::string m_settingsKey;
	std::string m_settingsValue;

	mutable std::string m_toolchainPreferenceName;
	mutable std::string m_workingDirectory;
	mutable std::string m_homeDirectory;
	mutable std::string m_globalSettingsFile;

	std::string m_initPath;
	std::string m_envFile;
	mutable std::string m_architectureRaw;
	std::string m_hostArchitecture;
	mutable std::string m_targetArchitecture;

	std::optional<uint> m_maxJobs;
	std::optional<bool> m_dumpAssembly;
	std::optional<bool> m_showCommands;
	std::optional<bool> m_benchmark;
	std::optional<bool> m_generateCompileCommands;

	Route m_command = Route::Unknown;
	IdeType m_generator = IdeType::None;
	SettingsType m_settingsType = SettingsType::None;

	bool m_saveSchemaToFile = false;
	mutable bool m_isMsvcPreRelease = false;
	mutable bool m_isToolchainPreset = false;
};
}

#endif // CHALET_COMMAND_LINE_INPUTS_HPP
