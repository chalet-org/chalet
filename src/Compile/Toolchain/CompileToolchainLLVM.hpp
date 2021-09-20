/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_COMPILE_TOOLCHAIN_LLVM_HPP
#define CHALET_COMPILE_TOOLCHAIN_LLVM_HPP

#include "Compile/Toolchain/CompileToolchainGNU.hpp"

namespace chalet
{
struct CompileToolchainLLVM : CompileToolchainGNU
{
	explicit CompileToolchainLLVM(const BuildState& inState, const SourceTarget& inProject, const CompilerConfig& inConfig);

	virtual ToolchainType type() const noexcept override;

	virtual StringList getLinkExclusions() const override;

	// Compile
	virtual void addWarnings(StringList& outArgList) const override;
	virtual void addProfileInformationCompileOption(StringList& outArgList) const override;
	virtual void addDiagnosticColorOption(StringList& outArgList) const override;
	virtual void addLibStdCppCompileOption(StringList& outArgList, const CxxSpecialization specialization) const override;
	virtual void addPositionIndependentCodeOption(StringList& outArgList) const override;
	virtual void addThreadModelCompileOption(StringList& outArgList) const override;
	virtual bool addArchitecture(StringList& outArgList) const override;
	virtual bool addArchitectureOptions(StringList& outArgList) const override;

	// Linking
	virtual void addLinks(StringList& outArgList) const override;
	virtual void addStripSymbolsOption(StringList& outArgList) const override;
	virtual void addLinkerScripts(StringList& outArgList) const override;
	virtual void addLibStdCppLinkerOption(StringList& outArgList) const override;
	virtual void addStaticCompilerLibraryOptions(StringList& outArgList) const override;
	virtual void addSubSystem(StringList& outArgList) const override;
	virtual void addEntryPoint(StringList& outArgList) const override;

	// Linking (Misc)
	virtual void startStaticLinkGroup(StringList& outArgList) const override;
	virtual void endStaticLinkGroup(StringList& outArgList) const override;
	virtual void startExplicitDynamicLinkGroup(StringList& outArgList) const override;
};
}

#endif // CHALET_COMPILE_TOOLCHAIN_LLVM_HPP
