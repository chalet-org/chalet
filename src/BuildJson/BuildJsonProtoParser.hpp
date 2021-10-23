/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_BUILD_JSON_PROTO_PARSER_HPP
#define CHALET_BUILD_JSON_PROTO_PARSER_HPP

#include "Libraries/Json.hpp"
#include "Json/JsonFile.hpp"

namespace chalet
{
struct CommandLineInputs;
struct StatePrototype;
struct JsonFile;
struct GitDependency;
struct BundleTarget;
struct ScriptDistTarget;

struct BuildJsonProtoParser
{
	explicit BuildJsonProtoParser(const CommandLineInputs& inInputs, StatePrototype& inPrototype);
	CHALET_DISALLOW_COPY_MOVE(BuildJsonProtoParser);
	~BuildJsonProtoParser();

	bool serialize() const;
	bool serializeDependenciesOnly() const;

private:
	bool validateAgainstSchema() const;
	bool serializeRequiredFromJsonRoot(const Json& inNode) const;

	bool parseRoot(const Json& inNode) const;

	bool parseConfiguration(const Json& inNode) const;

	bool parseDistribution(const Json& inNode) const;
	bool parseDistributionScript(ScriptDistTarget& outTarget, const Json& inNode) const;
	bool parseDistributionBundle(BundleTarget& outTarget, const Json& inNode) const;
	bool parseDistributionBundleLinux(BundleTarget& outTarget, const Json& inNode) const;
	bool parseDistributionBundleMacOS(BundleTarget& outTarget, const Json& inNode) const;
	bool parseDistributionBundleWindows(BundleTarget& outTarget, const Json& inNode) const;

	bool parseExternalDependencies(const Json& inNode) const;
	bool parseGitDependency(GitDependency& outDependency, const Json& inNode) const;
	bool parseTargetCondition(const Json& inNode) const;

	//
	template <typename T>
	bool parseKeyFromConfig(T& outVariable, const Json& inNode, const std::string& inKey) const;

	bool assignStringListFromConfig(StringList& outList, const Json& inNode, const std::string& inKey) const;

	bool conditionIsValid(const std::string& inContent) const;

	const CommandLineInputs& m_inputs;
	StatePrototype& m_prototype;
	JsonFile& m_chaletJson;
	const std::string& m_filename;

	const std::string kKeyDistribution = "distribution";
	const std::string kKeyConfigurations = "configurations";
	const std::string kKeyExternalDependencies = "externalDependencies";
};
}

#include "BuildJson/BuildJsonProtoParser.inl"

#endif // CHALET_BUILD_JSON_PROTO_PARSER_HPP
