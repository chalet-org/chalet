/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_VISUAL_STUDIO_COMPILE_ENVIRONMENT_HPP
#define CHALET_VISUAL_STUDIO_COMPILE_ENVIRONMENT_HPP

#include "Compile/Environment/CompileEnvironment.hpp"

namespace chalet
{
class BuildState;
struct CommandLineInputs;

class VisualStudioCompileEnvironment final : public CompileEnvironment
{
public:
	explicit VisualStudioCompileEnvironment(const CommandLineInputs& inInputs, BuildState& inState);

	static bool exists();

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

#endif // CHALET_VISUAL_STUDIO_COMPILE_ENVIRONMENT_HPP
