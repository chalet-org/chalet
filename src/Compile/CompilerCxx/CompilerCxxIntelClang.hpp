/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

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
