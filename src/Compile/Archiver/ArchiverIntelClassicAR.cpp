/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/Archiver/ArchiverIntelClassicAR.hpp"

#include "State/BuildState.hpp"
#include "State/Target/SourceTarget.hpp"

namespace chalet
{
/*****************************************************************************/
ArchiverIntelClassicAR::ArchiverIntelClassicAR(const BuildState& inState, const SourceTarget& inProject) :
	ArchiverGNUAR(inState, inProject)
{
}
}
