/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include "Compile/Linker/LinkerLLVMClang.hpp"

namespace chalet
{
struct LinkerEmscripten final : public LinkerLLVMClang
{
	explicit LinkerEmscripten(const BuildState& inState, const SourceTarget& inProject);

protected:
	virtual bool addExecutable(StringList& outArgList) const final;

	virtual void addLinks(StringList& outArgList) const final;
	virtual void addRunPath(StringList& outArgList) const final;
	virtual void addLinkerOptions(StringList& outArgList) const final;
	virtual void addThreadModelLinks(StringList& outArgList) const final;

	// Linking (Misc)
	virtual void addSharedOption(StringList& outArgList) const final;
	virtual void addExecutableOption(StringList& outArgList) const final;
	virtual void addPositionIndependentCodeOption(StringList& outArgList) const final;
};
}
