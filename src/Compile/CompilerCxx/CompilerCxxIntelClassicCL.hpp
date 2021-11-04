/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_COMPILER_EXECUTABLE_CXX_INTEL_CLASSIC_CL_HPP
#define CHALET_COMPILER_EXECUTABLE_CXX_INTEL_CLASSIC_CL_HPP

#include "Compile/CompilerCxx/CompilerCxxVisualStudioCL.hpp"

namespace chalet
{
struct CompilerCxxIntelClassicCL final : public CompilerCxxVisualStudioCL
{
	explicit CompilerCxxIntelClassicCL(const BuildState& inState, const SourceTarget& inProject);

	virtual bool initialize() final;

protected:
	virtual void addIncludes(StringList& outArgList) const final;
	virtual void addDiagnostics(StringList& outArgList) const final;
};
}

#endif // CHALET_COMPILER_EXECUTABLE_CXX_INTEL_CLASSIC_CL_HPP
