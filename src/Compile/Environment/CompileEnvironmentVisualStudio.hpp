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
	explicit CompileEnvironmentVisualStudio(const CommandLineInputs& inInputs, BuildState& inState);

	static bool exists();

	virtual ToolchainType type() const noexcept final;
	virtual StringList getVersionCommand(const std::string& inExecutable) const final;
	virtual std::string getFullCxxCompilerString(const std::string& inVersion) const final;
	virtual CompilerInfo getCompilerInfoFromExecutable(const std::string& inExecutable) const final;

	virtual bool compilerVersionIsToolchainVersion() const final;

protected:
	virtual bool createFromVersion(const std::string& inVersion) final;
	virtual bool validateArchitectureFromInput() final;

private:
	bool saveMsvcEnvironment() const;
	StringList getAllowedArchitectures() const;

	const std::string kVarsId;

	std::string m_varsFileOriginal;
	std::string m_varsFileMsvc;
	std::string m_varsFileMsvcDelta;

	std::string m_vsAppIdDir;

	bool m_msvcArchitectureSet = false;
};
}

#endif // CHALET_COMPILE_ENVIRONMENT_VISUAL_STUDIO_HPP
