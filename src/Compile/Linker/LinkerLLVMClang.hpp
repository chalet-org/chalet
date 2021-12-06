/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_LINKER_LLVM_CLANG_HPP
#define CHALET_LINKER_LLVM_CLANG_HPP

#include "Compile/Linker/LinkerGCC.hpp"

namespace chalet
{
struct LinkerLLVMClang : public LinkerGCC
{
	explicit LinkerLLVMClang(const BuildState& inState, const SourceTarget& inProject);

protected:
	virtual StringList getLinkExclusions() const override;

	virtual void addLinks(StringList& outArgList) const override;
	virtual void addStripSymbols(StringList& outArgList) const override;
	virtual void addLinkerScripts(StringList& outArgList) const override;
	virtual void addProfileInformation(StringList& outArgList) const override;
	virtual void addLibStdCppLinkerOption(StringList& outArgList) const override;
	virtual void addSanitizerOptions(StringList& outArgList) const override;
	virtual void addStaticCompilerLibraries(StringList& outArgList) const override;
	virtual void addSubSystem(StringList& outArgList) const override;
	virtual void addEntryPoint(StringList& outArgList) const override;
	virtual bool addArchitecture(StringList& outArgList, const std::string& inArch) const override;

	// Linking (Misc)
	virtual void startStaticLinkGroup(StringList& outArgList) const override;
	virtual void endStaticLinkGroup(StringList& outArgList) const override;
	virtual void startExplicitDynamicLinkGroup(StringList& outArgList) const override;
};
}

#endif // CHALET_LINKER_LLVM_CLANG_HPP
