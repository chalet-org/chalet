/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_COMPILE_TOOLCHAIN_APPLE_HPP
#define CHALET_COMPILE_TOOLCHAIN_APPLE_HPP

#include "Compile/Toolchain/CompileToolchainLLVM.hpp"

#include "BuildJson/ProjectConfiguration.hpp"
#include "Compile/CompilerConfig.hpp"
#include "State/BuildState.hpp"

namespace chalet
{
struct CompileToolchainApple final : CompileToolchainLLVM
{
	explicit CompileToolchainApple(const BuildState& inState, const ProjectConfiguration& inProject, const CompilerConfig& inConfig);

	virtual ToolchainType type() const final;

	// Linking
	virtual void addProfileInformationLinkerOption(StringList& inArgList) const final;
	virtual void addLibStdCppLinkerOption(StringList& inArgList) const final;

	// Objective-C / Objective-C++
	virtual void addObjectiveCxxLink(StringList& inArgList) const final;
	virtual void addObjectiveCxxCompileOption(StringList& inArgList, const CxxSpecialization specialization) const final;
	virtual void addObjectiveCxxRuntimeOption(StringList& inArgList, const CxxSpecialization specialization) const final;
};
}

#endif // CHALET_COMPILE_TOOLCHAIN_APPLE_HPP
