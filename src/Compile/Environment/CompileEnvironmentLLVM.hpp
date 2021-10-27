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
	explicit CompileEnvironmentLLVM(const ToolchainType inType, const CommandLineInputs& inInputs, BuildState& inState);

protected:
	virtual std::string getIdentifier() const noexcept override;
	virtual StringList getVersionCommand(const std::string& inExecutable) const override;
	virtual std::string getFullCxxCompilerString(const std::string& inVersion) const override;

	virtual bool makeArchitectureAdjustments() override;
	virtual bool validateArchitectureFromInput() override;
	virtual ToolchainType getToolchainTypeFromMacros(const std::string& inMacros) const override;
	virtual bool populateSupportedFlags(const std::string& inExecutable) override;
	virtual void parseSupportedFlagsFromHelpList(const StringList& inCommand) override;
};
}

#endif // CHALET_COMPILE_ENVIRONMENT_LLVM_HPP
