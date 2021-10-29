/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_ARCHIVER_INTEL_CLASSIC_AR_HPP
#define CHALET_ARCHIVER_INTEL_CLASSIC_AR_HPP

#include "Compile/Archiver/ArchiverGNUAR.hpp"

namespace chalet
{
struct ArchiverIntelClassicAR : public ArchiverGNUAR
{
	explicit ArchiverIntelClassicAR(const BuildState& inState, const SourceTarget& inProject);
};
}

#endif // CHALET_ARCHIVER_INTEL_CLASSIC_AR_HPP
