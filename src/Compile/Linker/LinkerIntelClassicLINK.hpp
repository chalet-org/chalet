/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_LINKER_INTEL_CLASSIC_LINK_HPP
#define CHALET_LINKER_INTEL_CLASSIC_LINK_HPP

#include "Compile/Linker/LinkerVisualStudioLINK.hpp"

namespace chalet
{
struct LinkerIntelClassicLINK : public LinkerVisualStudioLINK
{
	explicit LinkerIntelClassicLINK(const BuildState& inState, const SourceTarget& inProject);
};
}

#endif // CHALET_LINKER_INTEL_CLASSIC_LINK_HPP
