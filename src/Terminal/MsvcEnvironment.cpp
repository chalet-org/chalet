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
	if (m_initialized)
		return true;

	m_initialized = true;

	std::string progFiles;
	{
		auto envProgFiles = Environment::get("ProgramFiles(x86)");
		if (envProgFiles != nullptr)
			progFiles = std::string(envProgFiles);
	}

	if (progFiles.empty())
		return false;

	std::string vswhere = progFiles + "/Microsoft Visual Studio/Installer/vswhere.exe";
	if (!Commands::pathExists(vswhere))
		return false;

	m_vsAppIdDir = Commands::subprocessOutput({ progFiles + "/Microsoft Visual Studio/Installer/vswhere.exe", "-latest", "-property", "installationPath" });

	if (m_vsAppIdDir.empty() || !Commands::pathExists(m_vsAppIdDir))
		return false;

	if (!Commands::pathExists(m_varsFileMsvcDelta))
	{
		// Read the current environment and save it to a file
		saveOriginalEnvironment();

		// Read the MSVC environment and save it to a file
		saveMsvcEnvironment();

		// Get the delta between the two and save it to a file
		{
			std::ifstream msvcVarsInput(m_varsFileMsvc);
			std::string msvcVars((std::istreambuf_iterator<char>(msvcVarsInput)), std::istreambuf_iterator<char>());

			std::ifstream inputOrig(m_varsFileOriginal);
			for (std::string line; std::getline(inputOrig, line);)
			{
				String::replaceAll(msvcVars, line, "");
			}
			std::ofstream(m_varsFileMsvcDelta) << msvcVars;
			msvcVarsInput.close();
			inputOrig.close();
		}

		Commands::remove(m_varsFileOriginal);
		Commands::remove(m_varsFileMsvc);
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

	{
		auto include = m_variables.find("INCLUDE");
		if (include != m_variables.end())
		{
			String::replaceAll(include->second, "\\", "/");
			m_include = String::split(include->second, ";");
		}

		auto lib = m_variables.find("LIB");
		if (lib != m_variables.end())
		{
			String::replaceAll(lib->second, "\\", "/");
			m_lib = String::split(lib->second, ";");
		}
	}
#endif
	return true;
}

/*****************************************************************************/
const StringList& MsvcEnvironment::include() const noexcept
{
	return m_include;
}

/*****************************************************************************/
const StringList& MsvcEnvironment::lib() const noexcept
{
	return m_lib;
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
			fmt::format("\"{}\\VC\\Auxiliary\\Build\\vcvars64.bat\"", m_vsAppIdDir),
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
