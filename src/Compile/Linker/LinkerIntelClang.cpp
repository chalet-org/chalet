/*
	 Distributed under the OSI-approved BSD 3-Clause License.
	 See accompanying file LICENSE.txt for details.
 */

#include "Compile/Linker/LinkerIntelClang.hpp"

#include "State/BuildState.hpp"
#include "State/Target/SourceTarget.hpp"

namespace chalet
{
/*****************************************************************************/
LinkerIntelClang::LinkerIntelClang(const BuildState& inState, const SourceTarget& inProject) :
	LinkerLLVMClang(inState, inProject)
{
}

/*****************************************************************************/
void LinkerIntelClang::addThreadModelLinks(StringList& outArgList) const
{
	UNUSED(outArgList);
}
}
