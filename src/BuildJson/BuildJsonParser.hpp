/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_BUILD_JSON_PARSER_HPP
#define CHALET_BUILD_JSON_PARSER_HPP

#include "Libraries/Json.hpp"

namespace chalet
{
struct BundleTarget;
struct CommandLineInputs;
struct CMakeTarget;
struct JsonFile;
struct SourceTarget;
struct ScriptBuildTarget;
struct SubChaletTarget;
struct StatePrototype;
class BuildState;

struct BuildJsonParser
{
	explicit BuildJsonParser(const CommandLineInputs& inInputs, StatePrototype& inPrototype, BuildState& inState);
	CHALET_DISALLOW_COPY_MOVE(BuildJsonParser);
	~BuildJsonParser();

	bool serialize();

private:
	bool serializeFromJsonRoot(const Json& inJson);

	bool parseTarget(const Json& inNode);
	bool parseSourceTarget(SourceTarget& outProject, const Json& inNode, const bool inAbstract = false) const;
	bool parseScriptTarget(ScriptBuildTarget& outScript, const Json& inNode) const;
	bool parseSubChaletTarget(SubChaletTarget& outProject, const Json& inNode) const;
	bool parseCMakeTarget(CMakeTarget& outProject, const Json& inNode) const;
	bool parseTargetCondition(IBuildTarget& outTarget, const Json& inNode) const;
	bool parseCompilerSettingsCxx(SourceTarget& outProject, const Json& inNode) const;
	bool parseFilesAndLocation(SourceTarget& outProject, const Json& inNode, const bool inAbstract) const;
	bool parseProjectLocationOrFiles(SourceTarget& outProject, const Json& inNode) const;

	bool validBuildRequested() const;
	bool validRunTargetRequested() const;
	bool validRunTargetRequestedFromInput() const;

	template <typename T>
	bool parseKeyFromConfig(T& outVariable, const Json& inNode, const std::string& inKey) const;

	bool assignStringListFromConfig(StringList& outList, const Json& inNode, const std::string& inKey) const;

	bool containsComplexKey(const Json& inNode, const std::string& inKey) const;

	bool conditionIsValid(const std::string& inContent) const;

	const CommandLineInputs& m_inputs;
	JsonFile& m_chaletJson;
	const std::string& m_filename;
	BuildState& m_state;

	const std::string& kKeyAbstracts;
	const std::string& kKeyTargets;

	std::unordered_map<std::string, std::unique_ptr<SourceTarget>> m_abstractSourceTarget;

	std::string m_debugIdentifier{ "debug" };
};
}

#include "BuildJson/BuildJsonParser.inl"

#endif // CHALET_BUILD_JSON_PARSER_HPP
