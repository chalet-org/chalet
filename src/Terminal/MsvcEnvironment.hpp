/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_MSVC_ENVIRONMENT_HPP
#define CHALET_MSVC_ENVIRONMENT_HPP

#if defined(CHALET_WIN32)
	#include <unordered_map>
#endif

namespace chalet
{
class MsvcEnvironment
{
public:
	MsvcEnvironment() = default;

	bool readCompilerVariables();

	const StringList& include() const noexcept;
	const StringList& lib() const noexcept;

private:
	bool setVariableToPath(const char* inName);
	bool saveOriginalEnvironment();
	bool saveMsvcEnvironment();

#if defined(CHALET_WIN32)
	bool m_initialized = false;

	std::unordered_map<std::string, std::string> m_variables;

	std::string m_varsFileOriginal{ "build/variables_original.txt" };
	std::string m_varsFileMsvc{ "build/variables_msvc.txt" };
	std::string m_varsFileMsvcDelta{ "build/variables_msvc_delta.txt" };

	std::string m_vsAppIdDir;

#endif
	StringList m_include;
	StringList m_lib;
};
}

#endif // CHALET_MSVC_ENVIRONMENT_HPP
