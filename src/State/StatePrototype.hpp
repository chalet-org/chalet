/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_BUILD_JSON_PROTOTYPE_HPP
#define CHALET_BUILD_JSON_PROTOTYPE_HPP

#include "Libraries/Json.hpp"

#include "Cache/WorkspaceCache.hpp"
#include "SettingsJson/GlobalSettingsState.hpp"
#include "State/AncillaryTools.hpp"
#include "State/BuildConfiguration.hpp"
#include "State/Dependency/IBuildDependency.hpp"
#include "State/Distribution/IDistTarget.hpp"
#include "State/WorkspaceEnvironment.hpp"
#include "Json/JsonFile.hpp"

namespace chalet
{
struct CommandLineInputs;

struct StatePrototype
{
	explicit StatePrototype(CommandLineInputs& inInputs);

	bool initialize();
	bool validate();
	void saveCaches();

	bool initializeForList();

	JsonFile& chaletJson() noexcept;
	const JsonFile& chaletJson() const noexcept;
	const std::string& filename() const noexcept;

	const BuildConfigurationMap& buildConfigurations() const noexcept;
	const StringList& requiredBuildConfigurations() const noexcept;
	const std::string& releaseConfiguration() const noexcept;
	const std::string& anyConfiguration() const noexcept;

	WorkspaceEnvironment workspace;
	WorkspaceCache cache;
	AncillaryTools tools;
	DistributionTargetList distribution;
	BuildDependencyList externalDependencies;

private:
	friend struct BuildJsonProtoParser;

	bool createCache();

	bool parseEnvFile();
	bool parseGlobalSettingsJson();
	bool parseLocalSettingsJson();
	bool parseBuildJson();

	// bool validateConfigurations();
	bool validateExternalDependencies();
	bool validateBuildFile();

	bool runDependencyManager();
	bool validateBundleDestinations();

	bool makeDefaultBuildConfigurations();
	void addBuildConfiguration(const std::string&& inName, BuildConfiguration&& inConfig);
	void setReleaseConfiguration(const std::string& inName);
	void addRequiredBuildConfiguration(std::string inValue);

	CommandLineInputs& m_inputs;

	GlobalSettingsState m_globalSettingsState;
	BuildConfigurationMap m_buildConfigurations;

	StringList m_allowedBuildConfigurations;
	StringList m_requiredBuildConfigurations;

	std::string m_filename;
	std::string m_releaseConfiguration;

	JsonFile m_chaletJson;
};
}

#endif // CHALET_BUILD_JSON_PARSER_LITE_HPP
