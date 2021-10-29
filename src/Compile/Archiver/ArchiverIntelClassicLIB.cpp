/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/Archiver/ArchiverIntelClassicLIB.hpp"

#include "State/BuildState.hpp"
#include "State/Target/SourceTarget.hpp"

namespace chalet
{
/*****************************************************************************/
ArchiverIntelClassicLIB::ArchiverIntelClassicLIB(const BuildState& inState, const SourceTarget& inProject) :
	ArchiverVisualStudioLIB(inState, inProject)
{
}

}
