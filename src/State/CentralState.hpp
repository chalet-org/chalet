/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_CENTRAL_STATE_HPP
#define CHALET_CENTRAL_STATE_HPP

#include "Libraries/Json.hpp"

#include "Cache/WorkspaceCache.hpp"
#include "SettingsJson/IntermediateSettingsState.hpp"
#include "State/AncillaryTools.hpp"
#include "State/BuildConfiguration.hpp"
#include "State/Dependency/IBuildDependency.hpp"
#include "State/Distribution/IDistTarget.hpp"
#include "State/WorkspaceEnvironment.hpp"
#include "Json/JsonFile.hpp"

namespace chalet
{
struct CommandLineInputs;

struct CentralState
{
	explicit CentralState(CommandLineInputs& inInputs);

	bool initialize();
	bool validate();
	void saveCaches();

	bool initializeForList();

	const CommandLineInputs& inputs() const noexcept;
	JsonFile& chaletJson() noexcept;
	const JsonFile& chaletJson() const noexcept;
	const std::string& filename() const noexcept;

	const BuildConfigurationMap& buildConfigurations() const noexcept;
	const StringList& requiredBuildConfigurations() const noexcept;
	const std::string& releaseConfiguration() const noexcept;
	const std::string& anyConfiguration() const noexcept;

	void replaceVariablesInPath(std::string& outPath, const std::string& inName, const bool isDefine = false) const;
	void setRunArgumentMap(Dictionary<std::string>&& inMap);
	void getRunTargetArguments();

	WorkspaceEnvironment workspace;
	WorkspaceCache cache;
	AncillaryTools tools;
	DistributionTargetList distribution;
	BuildDependencyList externalDependencies;

private:
	friend struct CentralChaletJsonParser;

	bool createCache();

	bool parseEnvFile();
	bool parseGlobalSettingsJson();
	bool parseLocalSettingsJson();
	bool parseChaletJson();

	bool validateConfigurations();
	bool validateExternalDependencies();
	bool validateBuildFile();

	bool runDependencyManager();
	bool validateDistribution();

	bool makeDefaultBuildConfigurations();
	void addBuildConfiguration(const std::string& inName, BuildConfiguration&& inConfig);
	void setReleaseConfiguration(const std::string& inName);
	void addRequiredBuildConfiguration(std::string inValue);

	CommandLineInputs& m_inputs;

	IntermediateSettingsState m_globalSettingsState;
	BuildConfigurationMap m_buildConfigurations;

	Dictionary<std::string> m_runArgumentMap;

	StringList m_allowedBuildConfigurations;
	StringList m_requiredBuildConfigurations;

	std::string m_filename;
	std::string m_releaseConfiguration;

	JsonFile m_chaletJson;
};
}

#endif // CHALET_CENTRAL_STATE_HPP
