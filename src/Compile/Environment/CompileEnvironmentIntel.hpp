/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_COMPILE_ENVIRONMENT_INTEL_HPP
#define CHALET_COMPILE_ENVIRONMENT_INTEL_HPP

#include "Compile/Environment/CompileEnvironmentLLVM.hpp"

namespace chalet
{
struct CompileEnvironmentIntel final : CompileEnvironmentLLVM
{
	explicit CompileEnvironmentIntel(const ToolchainType inType, const CommandLineInputs& inInputs, BuildState& inState);

protected:
	virtual StringList getVersionCommand(const std::string& inExecutable) const final;
	virtual std::string getFullCxxCompilerString(const std::string& inVersion) const final;
	virtual bool createFromVersion(const std::string& inVersion) final;
	virtual bool validateArchitectureFromInput() final;
	virtual bool makeArchitectureAdjustments() final;
	virtual std::vector<CompilerPathStructure> getValidCompilerPaths() const final;

	virtual void parseVersionFromVersionOutput(const std::string& inLine, std::string& outVersion) const final;
	virtual ToolchainType getToolchainTypeFromMacros(const std::string& inMacros) const final;
	virtual bool populateSupportedFlags(const std::string& inExecutable) final;

private:
	bool saveIntelEnvironment() const;

	const std::string kVarsId;

	std::string m_varsFileOriginal;
	std::string m_varsFileIntel;
	std::string m_varsFileIntelDelta;
	std::string m_intelSetVars;
};
}

#endif // CHALET_COMPILE_ENVIRONMENT_INTEL_HPP
