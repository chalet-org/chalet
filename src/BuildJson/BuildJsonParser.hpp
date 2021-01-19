/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_BUILD_JSON_PARSER_HPP
#define CHALET_BUILD_JSON_PARSER_HPP

#include "Libraries/Json.hpp"

#include "BuildJson/ProjectConfiguration.hpp"
#include "State/BuildState.hpp"
#include "Json/JsonParser.hpp"

namespace chalet
{
struct BuildJsonParser : public JsonParser
{
	explicit BuildJsonParser(const CommandLineInputs& inInputs, BuildState& inState, std::string inFilename);

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
	bool parseProject(ProjectConfiguration& outProject, const Json& inNode, const bool inAllProjects = false);
	bool parseFilesAndLocation(ProjectConfiguration& outProject, const Json& inNode, const bool inAllProjects);
	bool parseProjectLocationOrFiles(ProjectConfiguration& outProject, const Json& inNode);
	bool parseProjectBeforeBuildScripts(ProjectConfiguration& outProject, const Json& inNode);
	bool parseProjectAfterBuildScripts(ProjectConfiguration& outProject, const Json& inNode);
	bool parseProjectCmake(ProjectConfiguration& outProject, const Json& inNode);
	bool parseProjectLangStandard(ProjectConfiguration& outProject, const Json& inNode);

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

	const CommandLineInputs& m_inputs;
	BuildState& m_state;

	const std::string kKeyBundle = "bundle";
	const std::string kKeyEnvironment = "environment";
	const std::string kKeyConfigurations = "configurations";
	const std::string kKeyExternalDependencies = "externalDependencies";
	const std::string kKeyProjects = "projects";

	const std::string kKeyAllProjects = "allProjects";

	std::string m_filename;
};
}

#include "BuildJson/BuildJsonParser.inl"

#endif // CHALET_BUILD_JSON_PARSER_HPP
