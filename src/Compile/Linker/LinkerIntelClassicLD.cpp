/*
	 Distributed under the OSI-approved BSD 3-Clause License.
	 See accompanying file LICENSE.txt for details.
 */

#include "Compile/Linker/LinkerIntelClassicLD.hpp"

#include "State/BuildState.hpp"
#include "State/Target/SourceTarget.hpp"

namespace chalet
{
/*****************************************************************************/
LinkerIntelClassicLD::LinkerIntelClassicLD(const BuildState& inState, const SourceTarget& inProject) :
	LinkerGNULD(inState, inProject)
{
}
}
