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
};
}

#endif // CHALET_COMPILE_TOOLCHAIN_APPLE_HPP
