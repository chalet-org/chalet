/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_CLION_WORKSPACE_GEN_HPP
#define CHALET_CLION_WORKSPACE_GEN_HPP

#include "Utility/Uuid.hpp"
#include "Xml/XmlFile.hpp"

namespace chalet
{
class BuildState;
struct IBuildTarget;
struct SourceTarget;

struct CLionWorkspaceGen
{
	explicit CLionWorkspaceGen(const std::vector<Unique<BuildState>>& inStates, const std::string& inDebugConfig, const std::string& inAllBuildName);

	bool saveToPath(const std::string& inPath);

private:
	struct RunConfiguration
	{
		std::string name;
		std::string config;
		std::string arch;
		std::string outputFile;
		std::string args;
		std::string path;
	};
	bool createCustomTargetsFile(const std::string& inFilename);
	bool createExternalToolsFile(const std::string& inFilename);
	bool createRunConfigurationFile(const std::string& inPath, const RunConfiguration& inRunConfig);
	bool createWorkspaceFile(const std::string& inFilename);
	bool createMiscFile(const std::string& inFilename);
	bool createJsonSchemasFile(const std::string& inFilename);

	BuildState& getDebugState() const;

	const std::string& getToolchain() const;
	StringList getArchitectures(const std::string& inToolchain) const;

	std::string getResolvedPath(const std::string& inFile) const;
	std::string getBoolString(const bool inValue) const;
	std::string getNodeIdentifier(const std::string& inName, const RunConfiguration& inRunConfig) const;
	std::string getTargetName(const RunConfiguration& inRunConfig) const;
	std::string getToolName(const std::string& inLabel, const RunConfiguration& inRunConfig) const;
	std::string getTargetFolderName(const RunConfiguration& inRunConfig) const;
	std::string getDefaultTargetName() const;

	const std::vector<Unique<BuildState>>& m_states;
	const std::string& m_debugConfiguration;
	const std::string& m_allBuildName;

	std::vector<RunConfiguration> m_runConfigs;
	std::map<std::string, std::string> m_toolsMap;
	StringList m_arches;

	std::string m_clionNamespaceGuid;
	std::string m_homeDirectory;
	std::string m_currentDirectory;
	std::string m_projectName;
	std::string m_defaultRunTargetName;
	std::string m_chaletPath;
	std::string m_projectId;
	std::string m_toolchain;

	std::string m_settingsFile;
	std::string m_inputFile;
};
}

#endif // CHALET_CLION_WORKSPACE_GEN_HPP
