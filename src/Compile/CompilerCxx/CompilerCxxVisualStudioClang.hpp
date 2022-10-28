/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_COMPILER_CXX_VISUAL_STUDIO_CLANG_HPP
#define CHALET_COMPILER_CXX_VISUAL_STUDIO_CLANG_HPP

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

#endif // CHALET_COMPILER_CXX_VISUAL_STUDIO_CLANG_HPP
