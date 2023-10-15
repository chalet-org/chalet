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
	explicit CLionWorkspaceGen(const std::vector<Unique<BuildState>>& inStates, const std::string& inDebugConfig);

	bool saveToPath(const std::string& inPath);

private:
	bool createCustomTargetsFile(const std::string& inFilename);
	bool createExternalToolsFile(const std::string& inFilename);
	bool createRunConfigurationFile(const std::string& inPath, const BuildState& inState, const SourceTarget& inTarget);
	bool createWorkspaceFile(const std::string& inFilename);

	BuildState& getDebugState() const;

	std::string getResolvedPath(const std::string& inFile) const;
	std::string getBoolString(const bool inValue) const;
	std::string getTargetName(const IBuildTarget& inTarget, const std::string& inConfig) const;

	const std::vector<Unique<BuildState>>& m_states;
	const std::string& m_debugConfiguration;

	std::map<std::string, std::string> m_toolsMap;

	std::string m_clionNamespaceGuid;
	std::string m_homeDirectory;
	std::string m_projectName;
	std::string m_chaletPath;
	std::string m_projectId;
};
}

#endif // CHALET_CLION_WORKSPACE_GEN_HPP
