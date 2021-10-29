/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_ARCHIVER_INTEL_CLASSIC_LIB_HPP
#define CHALET_ARCHIVER_INTEL_CLASSIC_LIB_HPP

#include "Compile/Archiver/ArchiverVisualStudioLIB.hpp"

namespace chalet
{
struct ArchiverIntelClassicLIB : public ArchiverVisualStudioLIB
{
	explicit ArchiverIntelClassicLIB(const BuildState& inState, const SourceTarget& inProject);
};
}

#endif // CHALET_ARCHIVER_INTEL_CLASSIC_LIB_HPP
