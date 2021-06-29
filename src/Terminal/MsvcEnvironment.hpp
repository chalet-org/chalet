/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_MSVC_ENVIRONMENT_HPP
#define CHALET_MSVC_ENVIRONMENT_HPP

#include "Core/CommandLineInputs.hpp"

#if defined(CHALET_WIN32)
	#include <unordered_map>
#endif

namespace chalet
{
class BuildState;
struct CommandLineInputs;

class MsvcEnvironment
{
public:
	explicit MsvcEnvironment(const CommandLineInputs& inInputs, BuildState& inState);

	static bool exists();

	bool create();

	const StringList& include() const noexcept;
	const StringList& lib() const noexcept;
	// const StringList& libPath() const noexcept;

private:
#if defined(CHALET_WIN32)
	static int s_exists;
	static std::string s_vswhere;
#endif

	bool setVariableToPath(const char* inName);
	bool saveOriginalEnvironment();
	bool saveMsvcEnvironment();

	const CommandLineInputs& m_inputs;
	BuildState& m_state;

#if defined(CHALET_WIN32)
	bool m_initialized = false;

	std::unordered_map<std::string, std::string> m_variables;

	std::string m_varsFileOriginal;
	std::string m_varsFileMsvc;
	std::string m_varsFileMsvcDelta;

	std::string m_vsAppIdDir;

#endif
	StringList m_include;
	StringList m_lib;
	// StringList m_libPath;
};
}

#endif // CHALET_MSVC_ENVIRONMENT_HPP
