/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Export/CodeBlocks/CodeBlocksCBPGen.hpp"

#include "Core/CommandLineInputs.hpp"
#include "State/BuildConfiguration.hpp"
#include "State/BuildState.hpp"
#include "State/Target/IBuildTarget.hpp"
#include "State/Target/SourceTarget.hpp"
#include "State/TargetMetadata.hpp"
#include "State/WorkspaceEnvironment.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
CodeBlocksCBPGen::CodeBlocksCBPGen(const std::vector<Unique<BuildState>>& inStates, const std::string& inAllBuildName) :
	m_states(inStates),
	m_allBuildName(inAllBuildName)
{
}

/*****************************************************************************/
bool CodeBlocksCBPGen::saveToFile(const std::string& inFilename)
{
	UNUSED(inFilename);
	return false;
}
}
