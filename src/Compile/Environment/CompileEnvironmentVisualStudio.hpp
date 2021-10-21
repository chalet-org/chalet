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

protected:
	virtual bool createFromVersion(const std::string& inVersion) final;

private:
	bool saveMsvcEnvironment() const;
	void makeArchitectureCorrections();

	const std::string kVarsId;

#if defined(CHALET_WIN32)
	std::string m_varsFileOriginal;
	std::string m_varsFileMsvc;
	std::string m_varsFileMsvcDelta;

	std::string m_vsAppIdDir;
#endif
};
}

#endif // CHALET_COMPILE_ENVIRONMENT_VISUAL_STUDIO_HPP
