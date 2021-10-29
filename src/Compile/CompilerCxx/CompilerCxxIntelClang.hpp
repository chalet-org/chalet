/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_COMPILER_EXECUTABLE_CXX_INTEL_CLANG_HPP
#define CHALET_COMPILER_EXECUTABLE_CXX_INTEL_CLANG_HPP

#include "Compile/CompilerCxx/CompilerCxxClang.hpp"

namespace chalet
{
struct CompilerCxxIntelClang : public CompilerCxxClang
{
	explicit CompilerCxxIntelClang(const BuildState& inState, const SourceTarget& inProject);

	virtual bool initialize() final;

private:
	// std::string m_pchSource;
};
}

#endif // CHALET_COMPILER_EXECUTABLE_CXX_INTEL_CLANG_HPP
