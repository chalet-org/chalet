/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_BUILD_JSON_PARSER_HPP
#define CHALET_BUILD_JSON_PARSER_HPP

#include "Libraries/Json.hpp"

#include "BuildJson/ProjectConfiguration.hpp"
#include "State/BuildState.hpp"
#include "State/CommandLineInputs.hpp"

namespace chalet
{
struct JsonFile;

struct BuildJsonParser
{
	explicit BuildJsonParser(const CommandLineInputs& inInputs, BuildState& inState, std::string inFilename);
	CHALET_DISALLOW_COPY_MOVE(BuildJsonParser);
	~BuildJsonParser();

	virtual bool serialize() final;

private:
	bool serializeFromJsonRoot(const Json& inJson);

	bool parseRoot(const Json& inNode);
	void parseBuildConfiguration(const Json& inNode);

	bool parseEnvironment(const Json& inNode);
	bool makePathVariable();

	bool parseConfiguration(const Json& inNode);

	bool parseExternalDependencies(const Json& inNode);
	bool parseExternalDependency(DependencyGit& outDependency, const Json& inNode);

	bool parseProjects(const Json& inNode);
	bool parseProject(ProjectConfiguration& outProject, const Json& inNode, const bool inAbstract = false);
	bool parseScript(ProjectConfiguration& outProject, const Json& inNode);
	bool parsePlatformConfigExclusions(ProjectConfiguration& outProject, const Json& inNode);
	bool parseCompilerSettingsCxx(ProjectConfiguration& outProject, const Json& inNode);
	bool parseFilesAndLocation(ProjectConfiguration& outProject, const Json& inNode, const bool inAbstract);
	bool parseProjectLocationOrFiles(ProjectConfiguration& outProject, const Json& inNode);
	bool parseProjectScripts(ProjectConfiguration& outProject, const Json& inNode);
	bool parseProjectCmake(ProjectConfiguration& outProject, const Json& inNode);

	bool parseBundle(const Json& inNode);
	bool parseBundleLinux(const Json& inNode);
	bool parseBundleMacOS(const Json& inNode);
	bool parseBundleWindows(const Json& inNode);

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
	const std::string kKeyEnvironment = "environment";
	const std::string kKeyConfigurations = "configurations";
	const std::string kKeyExternalDependencies = "externalDependencies";
	const std::string kKeyProjects = "projects";

	const std::string kKeyTemplates = "templates";

	std::unordered_map<std::string, std::unique_ptr<ProjectConfiguration>> m_abstractProjects;

	std::string m_filename;
	std::string m_debugIdentifier{ "debug" };

	std::unique_ptr<JsonFile> m_buildJson;
};
}

#include "BuildJson/BuildJsonParser.inl"

#endif // CHALET_BUILD_JSON_PARSER_HPP
