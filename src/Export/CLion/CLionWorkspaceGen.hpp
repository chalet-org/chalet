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

struct CLionWorkspaceGen
{
	explicit CLionWorkspaceGen(const std::vector<Unique<BuildState>>& inStates);

	bool saveToPath(const std::string& inPath);

private:
	bool createWorkspaceFile(const std::string& inFilename);

	const std::vector<Unique<BuildState>>& m_states;
};
}

#endif // CHALET_CLION_WORKSPACE_GEN_HPP
