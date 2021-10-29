/*
	 Distributed under the OSI-approved BSD 3-Clause License.
	 See accompanying file LICENSE.txt for details.
 */

#include "Compile/Linker/LinkerIntelClassicLINK.hpp"

#include "State/BuildState.hpp"
#include "State/Target/SourceTarget.hpp"

namespace chalet
{
/*****************************************************************************/
LinkerIntelClassicLINK::LinkerIntelClassicLINK(const BuildState& inState, const SourceTarget& inProject) :
	LinkerVisualStudioLINK(inState, inProject)
{
}
}
