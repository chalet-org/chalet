/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_COMPILE_TOOLCHAIN_LLVM_HPP
#define CHALET_COMPILE_TOOLCHAIN_LLVM_HPP

#include "Compile/Toolchain/CompileToolchainGNU.hpp"

#include "BuildJson/ProjectConfiguration.hpp"
#include "Compile/CompilerConfig.hpp"
#include "State/BuildState.hpp"

namespace chalet
{
struct CompileToolchainLLVM : CompileToolchainGNU
{
	explicit CompileToolchainLLVM(const BuildState& inState, const ProjectConfiguration& inProject, const CompilerConfig& inConfig);

	virtual ToolchainType type() const override;

	// Compile
	virtual void addProfileInformationCompileOption(StringList& inArgList) override;
	virtual void addLibStdCppCompileOption(StringList& inArgList, const CxxSpecialization specialization) override;
	virtual void addPositionIndependentCodeOption(StringList& inArgList) override;
	virtual void addThreadModelCompileOption(StringList& inArgList) override;

	// Linking
	virtual void addStripSymbolsOption(StringList& inArgList) override;
	virtual void addLinkerScripts(StringList& inArgList) override;
	virtual void addLibStdCppLinkerOption(StringList& inArgList) override;
	virtual void addStaticCompilerLibraryOptions(StringList& inArgList) override;
	virtual void addPlatformGuiApplicationFlag(StringList& inArgList) override;

	// Linking (Misc)
	virtual void startStaticLinkGroup(StringList& inArgList) override;
	virtual void endStaticLinkGroup(StringList& inArgList) override;
	virtual void startExplicitDynamicLinkGroup(StringList& inArgList) override;
};
}

#endif // CHALET_COMPILE_TOOLCHAIN_LLVM_HPP
