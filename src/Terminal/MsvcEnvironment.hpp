/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_MSVC_ENVIRONMENT_HPP
#define CHALET_MSVC_ENVIRONMENT_HPP

#include "Core/CommandLineInputs.hpp"

namespace chalet
{
class BuildState;
struct CommandLineInputs;

class MsvcEnvironment
{
public:
	explicit MsvcEnvironment(const CommandLineInputs& inInputs, BuildState& inState);

	static bool exists();

#if defined(CHALET_WIN32)
	const std::string& detectedVersion() const;
#endif

	bool create(const std::string& inVersion = std::string());

private:
	bool setVariableToPath(const char* inName);
	bool saveOriginalEnvironment();
	bool saveMsvcEnvironment();
	void makeArchitectureCorrections();
	std::string getMsvcVarsPath() const;

	const CommandLineInputs& m_inputs;
	BuildState& m_state;

#if defined(CHALET_WIN32)
	bool m_initialized = false;

	Dictionary<std::string> m_variables;

	std::string m_varsFileOriginal;
	std::string m_varsFileMsvc;
	std::string m_varsFileMsvcDelta;

	std::string m_vsAppIdDir;
	std::string m_detectedVersion;

#endif
};
}

#endif // CHALET_MSVC_ENVIRONMENT_HPP
