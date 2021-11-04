/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_LINKER_INTEL_LLD_HPP
#define CHALET_LINKER_INTEL_LLD_HPP

#include "Compile/Linker/LinkerLLVMLLD.hpp"

namespace chalet
{
struct LinkerIntelLLD final : public LinkerLLVMLLD
{
	explicit LinkerIntelLLD(const BuildState& inState, const SourceTarget& inProject);

protected:
	virtual void addThreadModelLinks(StringList& outArgList) const final;
};
}

#endif // CHALET_LINKER_INTEL_LLD_HPP
