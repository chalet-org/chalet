/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include "Compile/Environment/CompileEnvironmentLLVM.hpp"

namespace chalet
{
struct IntelEnvironmentScript;

struct CompileEnvironmentIntel final : CompileEnvironmentLLVM
{
	explicit CompileEnvironmentIntel(const ToolchainType inType, BuildState& inState);
	CHALET_DISALLOW_COPY_MOVE(CompileEnvironmentIntel);
	~CompileEnvironmentIntel();

protected:
	virtual StringList getVersionCommand(const std::string& inExecutable) const final;
	virtual std::string getFullCxxCompilerString(const std::string& inPath, const std::string& inVersion) const final;
	virtual bool createFromVersion(const std::string& inVersion) final;
	virtual bool validateArchitectureFromInput() final;
	virtual bool readArchitectureTripleFromCompiler() final;
	virtual std::vector<CompilerPathStructure> getValidCompilerPaths() const final;

	virtual void parseVersionFromVersionOutput(const std::string& inLine, std::string& outVersion) const final;
	virtual bool verifyCompilerExecutable(const std::string& inCompilerExec) final;
	virtual ToolchainType getToolchainTypeFromMacros(const std::string& inMacros) const final;
	virtual bool populateSupportedFlags(const std::string& inExecutable) final;

private:
	std::string makeToolchainName(const std::string& inArch) const;

	Unique<IntelEnvironmentScript> m_config;
};
}
