/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_COMPILE_ENVIRONMENT_LLVM_HPP
#define CHALET_COMPILE_ENVIRONMENT_LLVM_HPP

#include "Compile/Environment/CompileEnvironmentGNU.hpp"

namespace chalet
{
struct CompileEnvironmentLLVM : CompileEnvironmentGNU
{
	explicit CompileEnvironmentLLVM(const CommandLineInputs& inInputs, BuildState& inState);

	virtual ToolchainType type() const noexcept override;
	virtual StringList getVersionCommand(const std::string& inExecutable) const override;
	virtual std::string getFullCxxCompilerString(const std::string& inVersion) const override;

protected:
	virtual bool makeArchitectureAdjustments() override;
};
}

#endif // CHALET_COMPILE_ENVIRONMENT_LLVM_HPP
