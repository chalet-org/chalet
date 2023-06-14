/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_COMPILER_CXX_APPLE_CLANG_HPP
#define CHALET_COMPILER_CXX_APPLE_CLANG_HPP

#include "Compile/CompilerCxx/CompilerCxxClang.hpp"

namespace chalet
{
struct CompilerCxxAppleClang final : public CompilerCxxClang
{
	explicit CompilerCxxAppleClang(const BuildState& inState, const SourceTarget& inProject);

	static StringList getAllowedSDKTargets();

	static bool addSystemRootOption(StringList& outArgList, const BuildState& inState);
	static bool addArchitectureToCommand(StringList& outArgList, const BuildState& inState);
	static bool addMultiArchOptionsToCommand(StringList& outArgList, const std::string& inArch, const BuildState& inState);
	static void addSanitizerOptions(StringList& outArgList, const BuildState& inState);

protected:
	virtual void addProfileInformation(StringList& outArgList) const final;
	virtual void addSanitizerOptions(StringList& outArgList) const final;
	virtual void addPchInclude(StringList& outArgList, const SourceType derivative) const final;
	virtual bool addArchitecture(StringList& outArgList, const std::string& inArch) const final;
	virtual void addLibStdCppCompileOption(StringList& outArgList, const SourceType derivative) const final;
	virtual void addDiagnosticColorOption(StringList& outArgList) const final;

	virtual bool addSystemRootOption(StringList& outArgList) const final;
	virtual bool addSystemIncludes(StringList& outArgList) const final;

	// Objective-C / Objective-C++
	virtual void addObjectiveCxxRuntimeOption(StringList& outArgList, const SourceType derivative) const final;

private:
};
}

#endif // CHALET_COMPILER_CXX_APPLE_CLANG_HPP
