/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_BUILD_JSON_PARSER_HPP
#define CHALET_BUILD_JSON_PARSER_HPP

#include "Libraries/Json.hpp"

#include "Core/CommandLineInputs.hpp"
#include "State/BuildState.hpp"
#include "State/Dependency/GitDependency.hpp"

namespace chalet
{
struct JsonFile;
struct BundleTarget;
struct CMakeTarget;
struct ProjectTarget;
struct ScriptTarget;
struct SubChaletTarget;

struct BuildJsonParser
{
	explicit BuildJsonParser(const CommandLineInputs& inInputs, BuildState& inState, std::string inFilename);
	CHALET_DISALLOW_COPY_MOVE(BuildJsonParser);
	~BuildJsonParser();

	bool serialize();

private:
	bool serializeFromJsonRoot(const Json& inJson);

	bool parseRoot(const Json& inNode);
	void parseBuildConfiguration(const Json& inNode);

	bool makePathVariable();

	bool parseConfiguration(const Json& inNode);

	bool parseExternalDependencies(const Json& inNode);
	bool parseGitDependency(GitDependency& outDependency, const Json& inNode);

	bool parseProjects(const Json& inNode);
	bool parseProject(ProjectTarget& outProject, const Json& inNode, const bool inAbstract = false);
	bool parseScript(ScriptTarget& outScript, const Json& inNode);
	bool parseSubChaletTarget(SubChaletTarget& outProject, const Json& inNode);
	bool parseCMakeProject(CMakeTarget& outProject, const Json& inNode);
	bool parsePlatformConfigExclusions(IBuildTarget& outProject, const Json& inNode);
	bool parseCompilerSettingsCxx(ProjectTarget& outProject, const Json& inNode);
	bool parseFilesAndLocation(ProjectTarget& outProject, const Json& inNode, const bool inAbstract);
	bool parseProjectLocationOrFiles(ProjectTarget& outProject, const Json& inNode);

	bool parseDistribution(const Json& inNode);
	bool parseBundle(BundleTarget& outBundle, const Json& inNode);
	bool parseBundleLinux(BundleTarget& outBundle, const Json& inNode);
	bool parseBundleMacOS(BundleTarget& outBundle, const Json& inNode);
	bool parseBundleWindows(BundleTarget& outBundle, const Json& inNode);

	bool validBuildRequested();
	bool validRunProjectRequested();
	bool validRunProjectRequestedFromInput();
	bool setDefaultConfigurations(const std::string& inConfig);

	template <typename T>
	bool parseKeyFromConfig(T& outVariable, const Json& inNode, const std::string& inKey);

	bool assignStringFromConfig(std::string& outVariable, const Json& inNode, const std::string& inKey, const std::string& inDefault = "");
	bool assignStringListFromConfig(StringList& outList, const Json& inNode, const std::string& inKey);

	bool containsComplexKey(const Json& inNode, const std::string& inKey);
	bool containsKeyThatStartsWith(const Json& inNode, const std::string& inFind);

	const CommandLineInputs& m_inputs;
	BuildState& m_state;

	const std::string kKeyBundle = "bundle";
	const std::string kKeyConfigurations = "configurations";
	const std::string kKeyDistribution = "distribution";
	const std::string kKeyExternalDependencies = "externalDependencies";
	const std::string kKeyTargets = "targets";

	const std::string kKeyAbstracts = "abstracts";

	std::unordered_map<std::string, std::unique_ptr<ProjectTarget>> m_abstractProjects;

	std::string m_filename;
	std::string m_debugIdentifier{ "debug" };

	std::unique_ptr<JsonFile> m_buildJson;
};
}

#include "BuildJson/BuildJsonParser.inl"

#endif // CHALET_BUILD_JSON_PARSER_HPP
