/*
	 Distributed under the OSI-approved BSD 3-Clause License.
	 See accompanying file LICENSE.txt for details.
 */

#include "Compile/Linker/LinkerEmscripten.hpp"

#include "State/BuildPaths.hpp"
#include "State/BuildState.hpp"
#include "State/Target/SourceTarget.hpp"
#include "Utility/List.hpp"

namespace chalet
{
/*****************************************************************************/
LinkerEmscripten::LinkerEmscripten(const BuildState& inState, const SourceTarget& inProject) :
	LinkerLLVMClang(inState, inProject)
{
}

/*****************************************************************************/
void LinkerEmscripten::addLinks(StringList& outArgList) const
{
	LinkerLLVMClang::addLinks(outArgList);

	const auto& sharedLinks = m_project.projectSharedLinks();
	for (auto& target : m_state.targets)
	{
		if (target->isSources())
		{
			const auto& project = static_cast<const SourceTarget&>(*target);
			if (project.isSharedLibrary())
			{
				if (List::contains(sharedLinks, project.name()))
				{
					outArgList.emplace_back(m_state.paths.getTargetFilename(project));
				}
			}
		}
	}
}

/*****************************************************************************/
void LinkerEmscripten::addRunPath(StringList& outArgList) const
{
	UNUSED(outArgList);
}

/*****************************************************************************/
void LinkerEmscripten::addThreadModelLinks(StringList& outArgList) const
{
	UNUSED(outArgList);
}

/*****************************************************************************/
void LinkerEmscripten::addSharedOption(StringList& outArgList) const
{
	List::addIfDoesNotExist(outArgList, "-sSIDE_MODULE");
}

/*****************************************************************************/
void LinkerEmscripten::addExecutableOption(StringList& outArgList) const
{
	if (!m_project.projectSharedLinks().empty())
		List::addIfDoesNotExist(outArgList, "-sMAIN_MODULE");
	// UNUSED(outArgList);
}

/*****************************************************************************/
void LinkerEmscripten::addPositionIndependentCodeOption(StringList& outArgList) const
{
	List::addIfDoesNotExist(outArgList, "-fPIC");
}
}
