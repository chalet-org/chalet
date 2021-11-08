/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_COMPILE_ENVIRONMENT_VISUAL_STUDIO_HPP
#define CHALET_COMPILE_ENVIRONMENT_VISUAL_STUDIO_HPP

#include "Compile/Environment/ICompileEnvironment.hpp"

namespace chalet
{
class BuildState;
struct CommandLineInputs;

struct CompileEnvironmentVisualStudio final : ICompileEnvironment
{
	explicit CompileEnvironmentVisualStudio(const ToolchainType inType, const CommandLineInputs& inInputs, BuildState& inState);

	static bool exists();

protected:
	virtual std::string getIdentifier() const noexcept final;
	virtual StringList getVersionCommand(const std::string& inExecutable) const final;
	virtual std::string getFullCxxCompilerString(const std::string& inVersion) const final;
	virtual bool verifyToolchain() final;

	virtual bool compilerVersionIsToolchainVersion() const final;
	virtual bool createFromVersion(const std::string& inVersion) final;
	virtual bool validateArchitectureFromInput() final;
	virtual bool getCompilerVersionAndDescription(CompilerInfo& outInfo) const final;
	virtual std::vector<CompilerPathStructure> getValidCompilerPaths() const final;

private:
	std::string makeToolchainName() const;
	bool saveMsvcEnvironment() const;
	StringList getAllowedArchitectures() const;

	std::string m_varsFileOriginal;
	std::string m_varsFileMsvc;
	std::string m_varsFileMsvcDelta;

	std::string m_vsAppIdDir;
	std::string m_varsAllArch;

	bool m_msvcArchitectureSet = false;
};
}

#endif // CHALET_COMPILE_ENVIRONMENT_VISUAL_STUDIO_HPP
