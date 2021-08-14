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
struct ProjectTarget;
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

	bool parseProjects(const Json& inNode);
	bool parseProject(ProjectTarget& outProject, const Json& inNode, const bool inAbstract = false);
	bool parseScript(ScriptBuildTarget& outScript, const Json& inNode);
	bool parseSubChaletTarget(SubChaletTarget& outProject, const Json& inNode);
	bool parseCMakeProject(CMakeTarget& outProject, const Json& inNode);
	bool parsePlatformConfigExclusions(IBuildTarget& outProject, const Json& inNode);
	bool parseCompilerSettingsCxx(ProjectTarget& outProject, const Json& inNode);
	bool parseFilesAndLocation(ProjectTarget& outProject, const Json& inNode, const bool inAbstract);
	bool parseProjectLocationOrFiles(ProjectTarget& outProject, const Json& inNode);

	bool validBuildRequested();
	bool validRunProjectRequested();
	bool validRunProjectRequestedFromInput();

	template <typename T>
	bool parseKeyFromConfig(T& outVariable, const Json& inNode, const std::string& inKey);

	bool assignStringFromConfig(std::string& outVariable, const Json& inNode, const std::string& inKey, const std::string& inDefault = "");
	bool assignStringListFromConfig(StringList& outList, const Json& inNode, const std::string& inKey);

	bool containsComplexKey(const Json& inNode, const std::string& inKey);

	const CommandLineInputs& m_inputs;
	JsonFile& m_buildJson;
	const std::string& m_filename;
	BuildState& m_state;

	const std::string& kKeyAbstracts;
	const std::string& kKeyTargets;

	std::unordered_map<std::string, std::unique_ptr<ProjectTarget>> m_abstractProjects;

	std::string m_debugIdentifier{ "debug" };
};
}

#include "BuildJson/BuildJsonParser.inl"

#endif // CHALET_BUILD_JSON_PARSER_HPP
