/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_BUILD_JSON_PROTOTYPE_HPP
#define CHALET_BUILD_JSON_PROTOTYPE_HPP

#include "Libraries/Json.hpp"

#include "State/BuildConfiguration.hpp"
#include "State/BuildEnvironment.hpp"
#include "State/CacheTools.hpp"
#include "State/Distribution/IDistTarget.hpp"
#include "Json/JsonFile.hpp"

namespace chalet
{
struct CommandLineInputs;
struct BundleTarget;
struct ScriptDistTarget;

struct StatePrototype
{
	explicit StatePrototype(const CommandLineInputs& inInputs, std::string inFilename);

	bool initialize();
	bool validate();

	JsonFile& jsonFile() noexcept;
	const std::string& filename() const noexcept;

	const BuildConfigurationMap& buildConfigurations() const noexcept;
	const StringList& requiredBuildConfigurations() const noexcept;
	const StringList& requiredArchitectures() const noexcept;

	const std::string kKeyConfigurations = "configurations";
	const std::string kKeyDistribution = "distribution";
	const std::string kKeyExternalDependencies = "externalDependencies";
	const std::string kKeyTargets = "targets";

	const std::string kKeyAbstracts = "abstracts";

	BuildEnvironment environment;
	CacheTools tools;
	DistributionTargetList distribution;

private:
	bool validateBundleDestinations();
	bool parseRequired(const Json& inNode);
	bool parseConfiguration(const Json& inNode);
	bool parseDistribution(const Json& inNode);
	bool parseScript(ScriptDistTarget& outScript, const Json& inNode);
	bool parseBundle(BundleTarget& outBundle, const Json& inNode);
	bool parseBundleLinux(BundleTarget& outBundle, const Json& inNode);
	bool parseBundleMacOS(BundleTarget& outBundle, const Json& inNode);
	bool parseBundleWindows(BundleTarget& outBundle, const Json& inNode);

	bool makeDefaultBuildConfigurations();
	bool getDefaultBuildConfiguration(BuildConfiguration& outConfig, const std::string& inName) const;

	template <typename T>
	bool parseKeyFromConfig(T& outVariable, const Json& inNode, const std::string& inKey);

	bool assignStringFromConfig(std::string& outVariable, const Json& inNode, const std::string& inKey, const std::string& inDefault = "");
	bool assignStringListFromConfig(StringList& outList, const Json& inNode, const std::string& inKey);

	BuildConfigurationMap m_buildConfigurations;

	const CommandLineInputs& m_inputs;

	StringList m_allowedBuildConfigurations;
	StringList m_requiredBuildConfigurations;
	StringList m_requiredArchitectures;

	std::string m_filename;

	std::unique_ptr<JsonFile> m_buildJson;
};
}

#include "State/StatePrototype.inl"

#endif // CHALET_BUILD_JSON_PARSER_LITE_HPP
