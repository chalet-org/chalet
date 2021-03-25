/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Terminal/MsvcEnvironment.hpp"

#include "Libraries/Format.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Environment.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
bool MsvcEnvironment::readCompilerVariables()
{
#if defined(CHALET_WIN32)
	if (!Environment::isMsvc())
		return false;

	auto visualStudioPath = Environment::get("VSAPPIDDIR");
	// LOG(visualStudioPath);
	if (visualStudioPath == nullptr)
		return false;

	m_vsAppIdDir = std::string(visualStudioPath);

	if (!Commands::pathExists(m_varsFileMsvcDelta))
	{
		// Read the current environment and save it to a file
		saveOriginalEnvironment();

		// Read the MSVC environment and save it to a file
		saveMsvcEnvironment();

		// Get the delta between the two and save it to a file
		std::ifstream msvcVarsInput(m_varsFileMsvc);
		std::string msvcVars((std::istreambuf_iterator<char>(msvcVarsInput)), std::istreambuf_iterator<char>());

		std::ifstream inputOrig(m_varsFileOriginal);
		for (std::string line; std::getline(inputOrig, line);)
		{
			String::replaceAll(msvcVars, line, "");
		}
		std::ofstream(m_varsFileMsvcDelta) << msvcVars;
	}

	// Read delta to cache
	std::ifstream input(m_varsFileMsvcDelta);
	for (std::string line; std::getline(input, line);)
	{
		if (String::contains('=', line))
		{
			auto splitVar = String::split(line, '=');
			if (splitVar.size() == 2 && splitVar.front().size() > 0 && splitVar.back().size() > 0)
			{
				m_variables[splitVar.front()] = splitVar.back();
			}
		}
	}

	for (auto& [name, var] : m_variables)
	{
		Environment::set(name.c_str(), var);
	}

#endif
	return true;
}

/*****************************************************************************/
bool MsvcEnvironment::setVariableToPath(const char* inName)
{
#if defined(CHALET_WIN32)
	auto it = m_variables.find(inName);
	if (it != m_variables.end())
	{
		Environment::set(it->first.c_str(), it->second);
		return true;
	}
#else
	UNUSED(inName);
#endif

	return false;
}

/*****************************************************************************/
bool MsvcEnvironment::saveOriginalEnvironment()
{
#if defined(CHALET_WIN32)
	if (!Commands::pathExists(m_varsFileOriginal))
	{
		StringList cmd{
			"SET",
			">",
			m_varsFileOriginal
		};
		std::system(String::join(cmd).c_str());
		return true;
	}
#endif

	return false;
}

/*****************************************************************************/
bool MsvcEnvironment::saveMsvcEnvironment()
{
#if defined(CHALET_WIN32)
	if (!Commands::pathExists(m_varsFileMsvc))
	{
		// TODO: 32-bit arch would use vcvars32.bat
		StringList cmd{
			fmt::format("\"{}..\\..\\VC\\Auxiliary\\Build\\vcvars64.bat\"", m_vsAppIdDir),
			">",
			"nul",
			"&&",
			"SET",
			">",
			m_varsFileMsvc
		};
		std::system(String::join(cmd).c_str());
		return true;
	}
#endif

	return false;
}
}
