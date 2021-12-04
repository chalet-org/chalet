/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_LINKER_INTEL_CLANG_HPP
#define CHALET_LINKER_INTEL_CLANG_HPP

#include "Compile/Linker/LinkerLLVMClang.hpp"

namespace chalet
{
struct LinkerIntelClang final : public LinkerLLVMClang
{
	explicit LinkerIntelClang(const BuildState& inState, const SourceTarget& inProject);

protected:
	virtual void addThreadModelLinks(StringList& outArgList) const final;
};
}

#endif // CHALET_LINKER_INTEL_CLANG_HPP
