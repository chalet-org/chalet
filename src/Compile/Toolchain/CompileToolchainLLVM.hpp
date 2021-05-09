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
};
}

#endif // CHALET_COMPILE_TOOLCHAIN_LLVM_HPP
