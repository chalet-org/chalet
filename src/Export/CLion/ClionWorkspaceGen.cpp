/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Export/CLion/CLionWorkspaceGen.hpp"

#include "Terminal/Commands.hpp"

namespace chalet
{
/*****************************************************************************/
CLionWorkspaceGen::CLionWorkspaceGen(const std::vector<Unique<BuildState>>& inStates) :
	m_states(inStates)
{
}

/*****************************************************************************/
bool CLionWorkspaceGen::saveToPath(const std::string& inPath)
{
	UNUSED(inPath);

	auto toolsPath = fmt::format("{}/tools", inPath);

	if (!Commands::pathExists(toolsPath))
		Commands::makeDirectory(toolsPath);

	auto workspaceFile = fmt::format("{}/workspace.xml", inPath);
	auto customTargetsFile = fmt::format("{}/customTargets.xml", inPath);
	auto externalToolsFile = fmt::format("{}/External Tools.xml", toolsPath);

	LOG(workspaceFile);
	LOG(customTargetsFile);
	LOG(externalToolsFile);

	return false;
}

/*****************************************************************************/
bool CLionWorkspaceGen::createWorkspaceFile(const std::string& inFilename)
{
	UNUSED(inFilename);

	return false;
}
}
