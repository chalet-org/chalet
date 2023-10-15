/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_CODEBLOCKS_WORKSPACE_GEN_HPP
#define CHALET_CODEBLOCKS_WORKSPACE_GEN_HPP

#include "Utility/Uuid.hpp"
#include "Xml/XmlFile.hpp"

namespace chalet
{
class BuildState;
struct IBuildTarget;

struct CodeBlocksWorkspaceGen
{
	explicit CodeBlocksWorkspaceGen(const std::vector<Unique<BuildState>>& inStates, const std::string& inDebugConfig, const std::string& inAllBuildName);

	bool saveToFile(const std::string& inFilename);

private:
	bool createWorkspaceFile(const std::string& inFilename);
	bool createWorkspaceLayoutFile(const std::string& inFilename);

	BuildState& getDebugState() const;

	std::string getRelativeProjectPath(const std::string& inName) const;

	const std::vector<Unique<BuildState>>& m_states;
	const std::string& m_debugConfiguration;
	const std::string& m_allBuildName;
};
}

#endif // CHALET_CODEBLOCKS_WORKSPACE_GEN_HPP
