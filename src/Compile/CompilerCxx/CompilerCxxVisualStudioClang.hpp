/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include "Compile/CommandAdapter/CommandAdapterMSVC.hpp"
#include "Compile/CompilerCxx/CompilerCxxClang.hpp"

namespace chalet
{
struct CompilerCxxVisualStudioClang final : public CompilerCxxClang
{
	explicit CompilerCxxVisualStudioClang(const BuildState& inState, const SourceTarget& inProject);

protected:
	virtual void addDefines(StringList& outArgList) const final;

private:
	CommandAdapterMSVC m_msvcAdapter;
};
}
