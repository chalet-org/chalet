/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

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
	virtual void addAdditionalOptions(StringList& outArgList) const final;
	virtual void addNativeJustMyCodeDebugging(StringList& outArgList) const final;
	virtual void addAdditionalSecurityChecks(StringList& outArgList) const final;
	virtual void addExternalWarnings(StringList& outArgList) const final;
	virtual void addFastMathOption(StringList& outArgList) const final;
};
}
