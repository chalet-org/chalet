/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_COMPILE_ENVIRONMENT_LLVM_HPP
#define CHALET_COMPILE_ENVIRONMENT_LLVM_HPP

#include "Compile/Environment/ICompileEnvironment.hpp"

namespace chalet
{
struct CompileEnvironmentLLVM final : ICompileEnvironment
{
	explicit CompileEnvironmentLLVM(const CommandLineInputs& inInputs, BuildState& inState);

	virtual ToolchainType type() const noexcept final;
};
}

#endif // CHALET_COMPILE_ENVIRONMENT_LLVM_HPP
