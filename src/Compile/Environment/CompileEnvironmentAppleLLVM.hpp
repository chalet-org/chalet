/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_COMPILE_ENVIRONMENT_APPLE_LLVM_HPP
#define CHALET_COMPILE_ENVIRONMENT_APPLE_LLVM_HPP

#include "Compile/Environment/CompileEnvironmentLLVM.hpp"

namespace chalet
{
struct CompileEnvironmentAppleLLVM final : CompileEnvironmentLLVM
{
	explicit CompileEnvironmentAppleLLVM(const CommandLineInputs& inInputs, BuildState& inState);

	virtual ToolchainType type() const noexcept final;
	virtual std::string getFullCxxCompilerString(const std::string& inVersion) const final;
};
}

#endif // CHALET_COMPILE_ENVIRONMENT_APPLE_LLVM_HPP