/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include "Compile/CompilerCxx/CompilerCxxClang.hpp"

namespace chalet
{
struct CompilerCxxEmscripten : public CompilerCxxClang
{
	explicit CompilerCxxEmscripten(const BuildState& inState, const SourceTarget& inProject);

	virtual bool initialize() final;

protected:
	virtual bool addExecutable(StringList& outArgList) const final;

	virtual void addPositionIndependentCodeOption(StringList& outArgList) const final;
};
}
