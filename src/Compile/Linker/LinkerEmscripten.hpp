/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_LINKER_EMSCRIPTEN_HPP
#define CHALET_LINKER_EMSCRIPTEN_HPP

#include "Compile/Linker/LinkerLLVMClang.hpp"

namespace chalet
{
struct LinkerEmscripten final : public LinkerLLVMClang
{
	explicit LinkerEmscripten(const BuildState& inState, const SourceTarget& inProject);

protected:
	virtual void addRunPath(StringList& outArgList) const final;
	virtual void addThreadModelLinks(StringList& outArgList) const final;
};
}

#endif // CHALET_LINKER_EMSCRIPTEN_HPP
