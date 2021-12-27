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
	explicit CompileEnvironmentAppleLLVM(const ToolchainType inType, BuildState& inState);

protected:
	virtual std::string getFullCxxCompilerString(const std::string& inVersion) const final;
	virtual ToolchainType getToolchainTypeFromMacros(const std::string& inMacros) const override;
};
}

#endif // CHALET_COMPILE_ENVIRONMENT_APPLE_LLVM_HPP
