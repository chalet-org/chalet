/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_COMPILER_CXX_EMSCRIPTEN_HPP
#define CHALET_COMPILER_CXX_EMSCRIPTEN_HPP

#include "Compile/CompilerCxx/CompilerCxxClang.hpp"

namespace chalet
{
struct CompilerCxxEmscripten : public CompilerCxxClang
{
	explicit CompilerCxxEmscripten(const BuildState& inState, const SourceTarget& inProject);

	virtual bool initialize() final;

private:
	// std::string m_pchSource;
};
}

#endif // CHALET_COMPILER_CXX_EMSCRIPTEN_HPP
