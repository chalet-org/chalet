/*
	 Distributed under the OSI-approved BSD 3-Clause License.
	 See accompanying file LICENSE.txt for details.
 */

#include "Compile/Linker/LinkerIntelLLD.hpp"

#include "State/BuildState.hpp"
#include "State/Target/SourceTarget.hpp"

namespace chalet
{
/*****************************************************************************/
LinkerIntelLLD::LinkerIntelLLD(const BuildState& inState, const SourceTarget& inProject) :
	LinkerLLVMLLD(inState, inProject)
{
}

/*****************************************************************************/
void LinkerIntelLLD::addThreadModelLinkerOption(StringList& outArgList) const
{
	UNUSED(outArgList);
}
}
