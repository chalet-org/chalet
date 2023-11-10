/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include "Compile/Environment/CompileEnvironmentGNU.hpp"

namespace chalet
{
struct CompileEnvironmentLLVM : CompileEnvironmentGNU
{
	explicit CompileEnvironmentLLVM(const ToolchainType inType, BuildState& inState);

protected:
	virtual StringList getVersionCommand(const std::string& inExecutable) const override;
	virtual std::string getFullCxxCompilerString(const std::string& inPath, const std::string& inVersion) const override;

	virtual bool readArchitectureTripleFromCompiler() override;
	virtual bool validateArchitectureFromInput() override;
	virtual ToolchainType getToolchainTypeFromMacros(const std::string& inMacros) const override;
	virtual bool populateSupportedFlags(const std::string& inExecutable) override;
	virtual void parseSupportedFlagsFromHelpList(const StringList& inCommand) override;
};
}
