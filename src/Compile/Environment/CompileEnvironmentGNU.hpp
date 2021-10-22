/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_COMPILE_ENVIRONMENT_GNU_HPP
#define CHALET_COMPILE_ENVIRONMENT_GNU_HPP

#include "Compile/Environment/ICompileEnvironment.hpp"

namespace chalet
{
struct CompileEnvironmentGNU : ICompileEnvironment
{
	explicit CompileEnvironmentGNU(const CommandLineInputs& inInputs, BuildState& inState);

	virtual ToolchainType type() const noexcept override;
	virtual StringList getVersionCommand(const std::string& inExecutable) const override;
	virtual std::string getFullCxxCompilerString(const std::string& inVersion) const override;

	virtual bool makeArchitectureAdjustments() override;

protected:
	virtual bool validateArchitectureFromInput() override;
};
}

#endif // CHALET_COMPILE_ENVIRONMENT_GNU_HPP
