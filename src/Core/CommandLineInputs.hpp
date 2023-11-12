/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include "BuildEnvironment/VisualStudioVersion.hpp"
#include "Compile/ToolchainPreference.hpp"
#include "Core/Router/CommandRoute.hpp"
#include "Export/ExportKind.hpp"
#include "Init/InitTemplateType.hpp"
#include "Query/QueryOption.hpp"
#include "Settings/SettingsType.hpp"

namespace chalet
{
struct CommandLineInputs
{
	CommandLineInputs();

	void detectToolchainPreference() const;

	const std::string& defaultArchPreset() const noexcept;
	const std::string& defaultToolchainPreset() const noexcept;
	const std::string& defaultBuildStrategy() const noexcept;

	const std::string& workingDirectory() const noexcept;
	const std::string& homeDirectory() const noexcept;
	const std::string& globalSettingsFile() const noexcept;

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
	std::string getGlobalSettingsFilePath() const;
	std::string getGlobalDirectory() const;

	const std::string& rootDirectory() const noexcept;
	void setRootDirectory(std::string&& inValue) noexcept;

	const std::string& outputDirectory() const noexcept;
	void setOutputDirectory(std::string&& inValue) noexcept;

	const std::string& externalDirectory() const noexcept;
	void setExternalDirectory(std::string&& inValue) noexcept;

	const std::string& distributionDirectory() const noexcept;
	void setDistributionDirectory(std::string&& inValue) noexcept;

	const CommandRoute& route() const noexcept;
	void setRoute(const CommandRoute& inValue) noexcept;

	const std::string& buildConfiguration() const noexcept;
	void setBuildConfiguration(std::string&& inValue) noexcept;

	const std::string& lastTarget() const noexcept;
	void setLastTarget(std::string&& inValue) const noexcept;

	const std::optional<StringList>& runArguments() const noexcept;
	void setRunArguments(StringList&& inValue) const noexcept;
	void setRunArguments(const StringList& inValue) const noexcept;
	void setRunArguments(std::string&& inValue) const noexcept;

	const std::string& appPath() const noexcept;
	void setAppPath(const std::string& inValue) noexcept;

	ExportKind exportKind() const noexcept;
	const std::string& exportKindRaw() const noexcept;
	void setExportKind(std::string&& inValue) noexcept;

	const ToolchainPreference& toolchainPreference() const noexcept;
	void setToolchainPreference(std::string&& inValue) const noexcept;
	void setToolchainPreferenceType(const ToolchainType inValue) const noexcept;
	const std::string& toolchainPreferenceName() const noexcept;
	void setToolchainPreferenceName(std::string&& inValue) const noexcept;
	bool makeCustomToolchainFromEnvironment() const;

	const std::string& buildStrategyPreference() const noexcept;
	void setBuildStrategyPreference(std::string&& inValue);

	const std::string& buildPathStylePreference() const noexcept;
	void setBuildPathStylePreference(std::string&& inValue);

	VisualStudioVersion visualStudioVersion() const noexcept;
	bool isToolchainPreset() const noexcept;
	bool isMultiArchToolchainPreset() const noexcept;
	void setMultiArchToolchainPreset(const bool inValue) const noexcept;

	const std::string& initPath() const noexcept;
	void setInitPath(std::string&& inValue) noexcept;

	InitTemplateType initTemplate() const noexcept;
	void setInitTemplate(std::string&& inValue) noexcept;

	const std::string& envFile() const noexcept;
	void setEnvFile(std::string&& inValue) noexcept;
	std::string platformEnv() const noexcept;
	void resolveEnvFile();

	const std::string& architectureRaw() const noexcept;
	void setArchitectureRaw(std::string&& inValue) const noexcept;

	const std::string& hostArchitecture() const noexcept;
	const std::string& targetArchitecture() const noexcept;
	std::string getResolvedTargetArchitecture() const noexcept;
	void setTargetArchitecture(const std::string& inValue) const noexcept;

	const std::string& signingIdentity() const noexcept;
	void setSigningIdentity(std::string&& inValue) noexcept;

	const std::string& osTargetName() const noexcept;
	void setOsTargetName(std::string&& inValue) noexcept;
	std::string getDefaultOsTargetName() const;

	const std::string& osTargetVersion() const noexcept;
	void setOsTargetVersion(std::string&& inValue) noexcept;
	std::string getDefaultOsTargetVersion() const;

	const StringList& universalArches() const noexcept;

	const StringList& archOptions() const noexcept;
	void setArchOptions(StringList&& inList) const noexcept;
	std::string getArchWithOptionsAsString(const std::string& inArchBase) const;

	const StringList& commandList() const noexcept;
	void setCommandList(StringList&& inList) noexcept;

	const StringList& queryData() const noexcept;
	void setQueryData(StringList&& inList) noexcept;

	bool saveSchemaToFile() const noexcept;
	void setSaveSchemaToFile(const bool inValue) noexcept;

	QueryOption queryOption() const noexcept;
	void setQueryOption(std::string&& inValue) noexcept;

	SettingsType settingsType() const noexcept;
	void setSettingsType(const SettingsType inValue) noexcept;

	const std::string& settingsKey() const noexcept;
	void setSettingsKey(std::string&& inValue) noexcept;

	const std::string& settingsValue() const noexcept;
	void setSettingsValue(std::string&& inValue) noexcept;

	void clearWorkingDirectory(std::string& outValue) const;

	const std::optional<u32>& maxJobs() const noexcept;
	void setMaxJobs(const u32 inValue) noexcept;

	const std::optional<bool>& dumpAssembly() const noexcept;
	void setDumpAssembly(const bool inValue) noexcept;

	const std::optional<bool>& showCommands() const noexcept;
	void setShowCommands(const bool inValue) noexcept;

	const std::optional<bool>& benchmark() const noexcept;
	void setBenchmark(const bool inValue) noexcept;

	const std::optional<bool>& launchProfiler() const noexcept;
	void setLaunchProfiler(const bool inValue) noexcept;

	const std::optional<bool>& keepGoing() const noexcept;
	void setKeepGoing(const bool inValue) noexcept;

	const std::optional<bool>& generateCompileCommands() const noexcept;
	void setGenerateCompileCommands(const bool inValue) noexcept;

	const std::optional<bool>& onlyRequired() const noexcept;
	void setOnlyRequired(const bool inValue) noexcept;

	bool saveUserToolchainGlobally() const noexcept;
	void setSaveUserToolchainGlobally(const bool inValue) noexcept;

	StringList getToolchainPresets() const;
	StringList getExportKindPresets() const;
	StringList getProjectInitializationPresets() const;
	StringList getCliQueryOptions() const;

private:
	ToolchainPreference getToolchainPreferenceFromString(const std::string& inValue) const;
	ExportKind getExportKindFromString(const std::string& inValue) const;
	QueryOption getQueryOptionFromString(const std::string& inValue) const;
	VisualStudioVersion getVisualStudioVersionFromPresetString(const std::string& inValue) const;
	InitTemplateType getInitTemplateFromString(const std::string& inValue) const;
	std::string getValidGccArchTripleFromArch(const std::string& inArch) const;

	mutable ToolchainPreference m_toolchainPreference;

	mutable std::optional<StringList> m_runArguments;
	StringList m_commandList;
	StringList m_queryData;
	mutable StringList m_universalArches;
	mutable StringList m_archOptions;

	std::string m_inputFile;
	std::string m_settingsFile;
	std::string m_rootDirectory;
	std::string m_outputDirectory;
	std::string m_externalDirectory;
	std::string m_distributionDirectory;
	std::string m_buildConfiguration;
	std::string m_buildFromCommandLine;
	mutable std::string m_lastTarget;
	std::string m_appPath;
	std::string m_exportKindRaw;
	std::string m_settingsKey;
	std::string m_settingsValue;
	std::string m_buildStrategyPreference;
	std::string m_buildPathStylePreference;
	std::string m_signingIdentity;
	std::string m_osTargetName;
	std::string m_osTargetVersion;

	mutable std::string m_toolchainPreferenceName;
	mutable std::string m_workingDirectory;
	mutable std::string m_homeDirectory;

	std::string m_initPath;
	std::string m_envFile;
	mutable std::string m_architectureRaw;
	std::string m_hostArchitecture;
	mutable std::string m_targetArchitecture;

	std::optional<u32> m_maxJobs;
	std::optional<bool> m_dumpAssembly;
	std::optional<bool> m_showCommands;
	std::optional<bool> m_benchmark;
	std::optional<bool> m_launchProfiler;
	std::optional<bool> m_keepGoing;
	std::optional<bool> m_generateCompileCommands;
	std::optional<bool> m_onlyRequired;

	CommandRoute m_route;

	ExportKind m_exportKind = ExportKind::None;
	SettingsType m_settingsType = SettingsType::Local;
	QueryOption m_queryOption = QueryOption::None;
	InitTemplateType m_initTemplate = InitTemplateType::None;

	mutable VisualStudioVersion m_visualStudioVersion = VisualStudioVersion::None;

	bool m_saveSchemaToFile = false;
	mutable bool m_isToolchainPreset = false;
	mutable bool m_isMultiArchToolchainPreset = false;
	bool m_saveUserToolchainGlobally = false;
};
}
