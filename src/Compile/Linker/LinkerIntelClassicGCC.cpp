/*
	 Distributed under the OSI-approved BSD 3-Clause License.
	 See accompanying file LICENSE.txt for details.
 */

#include "Compile/Linker/LinkerIntelClassicGCC.hpp"

#include "State/BuildState.hpp"
#include "State/Target/SourceTarget.hpp"

namespace chalet
{
/*****************************************************************************/
LinkerIntelClassicGCC::LinkerIntelClassicGCC(const BuildState& inState, const SourceTarget& inProject) :
	LinkerGCC(inState, inProject)
{
}
}
