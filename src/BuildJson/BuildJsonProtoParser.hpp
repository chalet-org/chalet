/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_BUILD_JSON_PROTO_PARSER_HPP
#define CHALET_BUILD_JSON_PROTO_PARSER_HPP

#include "Libraries/Json.hpp"

namespace chalet
{
struct CommandLineInputs;
struct StatePrototype;
struct JsonFile;
struct BundleTarget;
struct ScriptDistTarget;

struct BuildJsonProtoParser
{
	explicit BuildJsonProtoParser(const CommandLineInputs& inInputs, StatePrototype& inPrototype);
	CHALET_DISALLOW_COPY_MOVE(BuildJsonProtoParser);
	~BuildJsonProtoParser();

	bool serialize();

private:
	bool serializeRequiredFromJsonRoot(const Json& inNode);
	bool parseConfiguration(const Json& inNode);
	bool parseDistribution(const Json& inNode);
	bool parseScript(ScriptDistTarget& outScript, const Json& inNode);
	bool parseBundle(BundleTarget& outBundle, const Json& inNode);
	bool parseBundleLinux(BundleTarget& outBundle, const Json& inNode);
	bool parseBundleMacOS(BundleTarget& outBundle, const Json& inNode);
	bool parseBundleWindows(BundleTarget& outBundle, const Json& inNode);

	//
	template <typename T>
	bool parseKeyFromConfig(T& outVariable, const Json& inNode, const std::string& inKey);

	bool assignStringFromConfig(std::string& outVariable, const Json& inNode, const std::string& inKey, const std::string& inDefault = "");
	bool assignStringListFromConfig(StringList& outList, const Json& inNode, const std::string& inKey);

	const CommandLineInputs& m_inputs;
	StatePrototype& m_prototype;
	JsonFile& m_buildJson;
	const std::string& m_filename;

	const std::string kKeyDistribution = "distribution";
	const std::string kKeyConfigurations = "configurations";
};
}

#include "BuildJson/BuildJsonProtoParser.inl"

#endif // CHALET_BUILD_JSON_PROTO_PARSER_HPP
