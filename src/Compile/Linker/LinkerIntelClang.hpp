/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

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
