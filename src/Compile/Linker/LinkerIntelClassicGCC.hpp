/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_LINKER_INTEL_CLASIC_GCC_HPP
#define CHALET_LINKER_INTEL_CLASIC_GCC_HPP

#include "Compile/Linker/LinkerGCC.hpp"

namespace chalet
{
struct LinkerIntelClassicGCC final : public LinkerGCC
{
	explicit LinkerIntelClassicGCC(const BuildState& inState, const SourceTarget& inProject);
};
}

#endif // CHALET_LINKER_INTEL_CLASIC_GCC_HPP
