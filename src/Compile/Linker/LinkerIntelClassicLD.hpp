/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_LINKER_INTEL_CLASIC_LD_HPP
#define CHALET_LINKER_INTEL_CLASIC_LD_HPP

#include "Compile/Linker/LinkerGNULD.hpp"

namespace chalet
{
struct LinkerIntelClassicLD final : public LinkerGNULD
{
	explicit LinkerIntelClassicLD(const BuildState& inState, const SourceTarget& inProject);
};
}

#endif // CHALET_LINKER_INTEL_CLASIC_LD_HPP
