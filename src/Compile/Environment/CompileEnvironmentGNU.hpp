/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_COMPILE_ENVIRONMENT_GNU_HPP
#define CHALET_COMPILE_ENVIRONMENT_GNU_HPP

#include "Compile/Environment/ICompileEnvironment.hpp"
#include "Process/PipeOption.hpp"

namespace chalet
{
struct CompileEnvironmentGNU : ICompileEnvironment
{
	explicit CompileEnvironmentGNU(const ToolchainType inType, BuildState& inState);

	virtual void generateTargetSystemPaths() override;

protected:
	virtual StringList getVersionCommand(const std::string& inExecutable) const override;
	virtual std::string getFullCxxCompilerString(const std::string& inVersion) const override;
	virtual bool verifyToolchain() override;
	virtual bool supportsFlagFile() override;

	virtual bool readArchitectureTripleFromCompiler() override;
	virtual bool validateArchitectureFromInput() override;
	virtual bool getCompilerVersionAndDescription(CompilerInfo& outInfo) const final;
	virtual std::vector<CompilerPathStructure> getValidCompilerPaths() const override;
	virtual bool populateSupportedFlags(const std::string& inExecutable) override;

	virtual void parseVersionFromVersionOutput(const std::string& inLine, std::string& outVersion) const;
	virtual void parseArchFromVersionOutput(const std::string& inLine, std::string& outArch) const;
	virtual void parseThreadModelFromVersionOutput(const std::string& inLine, std::string& outThreadModel) const;
	virtual bool verifyCompilerExecutable(const std::string& inCompilerExec);
	virtual ToolchainType getToolchainTypeFromMacros(const std::string& inMacros) const;
	virtual void parseSupportedFlagsFromHelpList(const StringList& inCommand);

private:
	std::string getCompilerMacros(const std::string& inCompilerExec, const PipeOption inStdError = PipeOption::Close);

	// bool m_genericGcc = false;
};
}

#endif // CHALET_COMPILE_ENVIRONMENT_GNU_HPP
