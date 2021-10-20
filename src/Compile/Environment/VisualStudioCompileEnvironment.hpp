/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_VISUAL_STUDIO_COMPILE_ENVIRONMENT_HPP
#define CHALET_VISUAL_STUDIO_COMPILE_ENVIRONMENT_HPP

#include "Compile/Environment/CompileEnvironment.hpp"
#include "Core/CommandLineInputs.hpp"

namespace chalet
{
class BuildState;
struct CommandLineInputs;

class VisualStudioCompileEnvironment final : public CompileEnvironment
{
public:
	explicit VisualStudioCompileEnvironment(const CommandLineInputs& inInputs, BuildState& inState);

	static bool exists();

	virtual bool create(const std::string& inVersion = std::string()) final;

private:
	bool setVariableToPath(const char* inName);
	bool saveOriginalEnvironment();
	bool saveMsvcEnvironment();
	void makeArchitectureCorrections();
	std::string getMsvcVarsPath() const;

#if defined(CHALET_WIN32)
	bool m_initialized = false;

	Dictionary<std::string> m_variables;

	std::string m_varsFileOriginal;
	std::string m_varsFileMsvc;
	std::string m_varsFileMsvcDelta;

	std::string m_vsAppIdDir;
#endif
};
}

#endif // CHALET_VISUAL_STUDIO_COMPILE_ENVIRONMENT_HPP
