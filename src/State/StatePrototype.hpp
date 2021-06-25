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
#include "State/Distribution/IDistTarget.hpp"
#include "State/WorkspaceEnvironment.hpp"
#include "Json/JsonFile.hpp"

namespace chalet
{
struct CommandLineInputs;

struct StatePrototype
{
	explicit StatePrototype(const CommandLineInputs& inInputs, std::string inFilename);

	bool initialize();
	bool validate();
	void saveCaches();

	JsonFile& jsonFile() noexcept;
	const std::string& filename() const noexcept;

	const BuildConfigurationMap& buildConfigurations() const noexcept;
	const StringList& requiredBuildConfigurations() const noexcept;
	const StringList& requiredArchitectures() const noexcept;
	const std::string& releaseConfiguration() const noexcept;
	const std::string& anyConfiguration() const noexcept;

	const std::string kKeyExternalDependencies = "externalDependencies";
	const std::string kKeyTargets = "targets";
	const std::string kKeyAbstracts = "abstracts";

	WorkspaceEnvironment environment;
	WorkspaceCache cache;
	AncillaryTools tools;
	DistributionTargetList distribution;

private:
	friend struct BuildJsonProtoParser;

	bool parseGlobalSettingsJson();
	bool parseLocalSettingsJson();
	bool parseBuildJson();

	bool validateBundleDestinations();

	bool makeDefaultBuildConfigurations();
	bool getDefaultBuildConfiguration(BuildConfiguration& outConfig, const std::string& inName) const;
	void addBuildConfiguration(const std::string&& inName, BuildConfiguration&& inConfig);
	void setReleaseConfiguration(const std::string& inName);
	void addRequiredArchitecture(std::string inArch);

	GlobalSettingsState m_globalSettingsState;
	BuildConfigurationMap m_buildConfigurations;

	const CommandLineInputs& m_inputs;

	StringList m_allowedBuildConfigurations;
	StringList m_requiredBuildConfigurations;
	StringList m_requiredArchitectures;

	std::string m_filename;
	std::string m_releaseConfiguration;

	std::unique_ptr<JsonFile> m_buildJson;
};
}

#endif // CHALET_BUILD_JSON_PARSER_LITE_HPP
