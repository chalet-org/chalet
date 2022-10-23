/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_COMPILER_EXECUTABLE_CXX_CLANG_HPP
#define CHALET_COMPILER_EXECUTABLE_CXX_CLANG_HPP

#include "Compile/CompilerCxx/CompilerCxxGCC.hpp"

namespace chalet
{
struct CompilerCxxClang : public CompilerCxxGCC
{
	explicit CompilerCxxClang(const BuildState& inState, const SourceTarget& inProject);

	static bool addArchitectureToCommand(StringList& outArgList, const std::string& inArch, const BuildState& inState);
	static void addSanitizerOptions(StringList& outArgList, const BuildState& inState);

protected:
	virtual std::string getPragmaId() const override;
	virtual StringList getWarningExclusions() const override;

	virtual void addWarnings(StringList& outArgList) const override;
	virtual void addProfileInformation(StringList& outArgList) const override;
	virtual void addSanitizerOptions(StringList& outArgList) const override;
	virtual void addDiagnosticColorOption(StringList& outArgList) const override;
	virtual void addLibStdCppCompileOption(StringList& outArgList, const SourceType derivative) const override;
	virtual void addPositionIndependentCodeOption(StringList& outArgList) const override;
	virtual void addThreadModelCompileOption(StringList& outArgList) const override;
	virtual bool addArchitecture(StringList& outArgList, const std::string& inArch) const override;

	virtual void addLinkTimeOptimizations(StringList& outArgList) const override;
	virtual void addCppCoroutines(StringList& outArgList) const override;
	virtual void addCppConcepts(StringList& outArgList) const override;
};
}

#endif // CHALET_COMPILER_EXECUTABLE_CXX_CLANG_HPP
