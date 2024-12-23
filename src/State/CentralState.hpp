/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include "Libraries/Json.hpp"

#include "Cache/WorkspaceCache.hpp"
#include "State/AncillaryTools.hpp"
#include "State/BuildConfiguration.hpp"
#include "State/Dependency/IExternalDependency.hpp"
#include "State/Distribution/IDistTarget.hpp"
#include "State/WorkspaceEnvironment.hpp"
#include "Json/JsonFile.hpp"

namespace chalet
{
struct CommandLineInputs;
struct IntermediateSettingsState;

struct CentralState
{
	explicit CentralState(CommandLineInputs& inInputs);

	bool initialize();
	bool validate();
	void saveCaches();

	bool initializeForQuery();

	const CommandLineInputs& inputs() const noexcept;
	JsonFile& chaletJson() noexcept;
	const JsonFile& chaletJson() const noexcept;
	const std::string& filename() const noexcept;

	const BuildConfigurationMap& buildConfigurations() const noexcept;

	void setRunArgumentMap(Dictionary<StringList>&& inMap);
	void setRunArguments(const std::string& inKey, StringList&& inValue);
	void addRunArgumentsIfNew(const std::string& inKey, StringList&& inValue);
	const Dictionary<StringList>& runArgumentMap() const noexcept;
	const std::optional<StringList>& getRunTargetArguments(const std::string& inTarget) const;
	void clearRunArgumentMap();

	bool shouldPerformUpdateCheck() const;

	bool isAllowedArchitecture(const std::string& inArch, const bool inError = true) const;

	bool replaceVariablesInString(std::string& outString, const IExternalDependency* inTarget, const bool inCheckHome = true, const std::function<std::string(std::string)>& onFail = nullptr) const;

	StringList getArgumentStringListFromString(const std::string& inValue);

	WorkspaceEnvironment workspace;
	WorkspaceCache cache;
	AncillaryTools tools;
	ExternalDependencyList externalDependencies;

private:
	friend struct CentralChaletJsonParser;
	friend struct GlobalSettingsJsonParser;

	bool createCache();

	bool parseEnvFile();
	bool parseGlobalSettingsJson(IntermediateSettingsState& outState);
	bool parseLocalSettingsJson(const IntermediateSettingsState& inState);
	bool parseBuildFile();

	bool validateOsTarget();
	bool validateConfigurations();
	bool validateExternalDependencies();
	bool validateAncillaryTools();

	bool runDependencyManager();

	bool makeDefaultBuildConfigurations();
	void addBuildConfiguration(const std::string& inName, BuildConfiguration&& inConfig);

	void addAllowedArchitecture(std::string&& inArch);

	void shouldCheckForUpdate(const time_t inLastUpdate, const time_t inCurrent);

	CommandLineInputs& m_inputs;

	BuildConfigurationMap m_buildConfigurations;

	Dictionary<StringList> m_runArgumentMap;

	StringList m_allowedArchitectures;
	StringList m_requiredBuildConfigurations;

	std::string m_filename;

	JsonFile m_chaletJson;

	bool m_shouldPerformUpdateCheck = true;
};
}
