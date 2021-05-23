/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Terminal/MsvcEnvironment.hpp"

#include "Libraries/Format.hpp"
#include "State/BuildPaths.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Environment.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
MsvcEnvironment::MsvcEnvironment(BuildPaths& inPath) :
	m_path(inPath)
{
#if !defined(CHALET_WIN32)
	UNUSED(m_path);
#endif
}

/*****************************************************************************/
bool MsvcEnvironment::readCompilerVariables()
{
#if defined(CHALET_WIN32)
	if (m_initialized)
		return true;

	auto& buildDir = m_path.buildDir();

	m_varsFileOriginal = buildDir + "/original.env";
	m_varsFileMsvc = buildDir + "/msvc_all.env";
	m_varsFileMsvcDelta = buildDir + "/msvc.env";

	m_initialized = true;

	auto path = Environment::getPath();

	if (!Commands::pathExists(m_varsFileMsvcDelta))
	{
		std::string progFiles;
		{
			auto envProgFiles = Environment::get("ProgramFiles(x86)");
			if (envProgFiles != nullptr)
				progFiles = std::string(envProgFiles);
		}

		if (progFiles.empty())
		{
			Diagnostic::error("MSVC Environment could not be fetched: Error reading %ProgramFiles(x86%)");
			return false;
		}

		std::string vswhere = fmt::format("{}\\Microsoft Visual Studio\\Installer\\vswhere.exe", progFiles);
		if (!Commands::pathExists(vswhere))
		{
			Diagnostic::error("MSVC Environment could not be fetched: vswhere.exe could not be found");
			return false;
		}

		m_vsAppIdDir = Commands::subprocessOutput({ vswhere, "-latest", "-property", "installationPath" });
		if (m_vsAppIdDir.empty() || !Commands::pathExists(m_vsAppIdDir))
		{
			Diagnostic::error("MSVC Environment could not be fetched: Error running vswhere.exe");
			return false;
		}

		// Read the current environment and save it to a file
		if (!saveOriginalEnvironment())
		{
			Diagnostic::error("MSVC Environment could not be fetched: Error reading from the inherited environment.");
			return false;
		}

		// Read the MSVC environment and save it to a file
		if (!saveMsvcEnvironment())
		{
			Diagnostic::error("MSVC Environment could not be fetched: Error saving the full MSVC environment.");
			return false;
		}

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

		{
			std::string outContents;
			std::ifstream input(m_varsFileMsvcDelta);
			for (std::string line; std::getline(input, line);)
			{
				if (!line.empty())
				{
					if (String::startsWith("PATH=", line) || String::startsWith("Path=", line))
					{
						String::replaceAll(line, path, "");
					}
					outContents += line + "\n";
				}
			}
			input.close();
			std::ofstream(m_varsFileMsvcDelta) << outContents;
		}
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
		if (String::equals("PATH", name) || String::equals("Path", name))
		{
			Environment::set(name.c_str(), var + ";" + path);
		}
		else
		{
			Environment::set(name.c_str(), var);
		}
	}

	if (m_vsAppIdDir.empty())
	{
		auto vsappDir = m_variables.find("VSINSTALLDIR");
		if (vsappDir != m_variables.end())
		{
			m_vsAppIdDir = vsappDir->second;
		}
	}

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
#endif
	return true;
}

/*****************************************************************************/
void MsvcEnvironment::cleanup()
{
	Commands::remove(m_varsFileMsvcDelta);
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
	StringList cmd{
		"cmd.exe",
		"/c",
		"SET"
	};
	bool result = Commands::subprocessOutputToFile(cmd, m_varsFileOriginal);

	return result;
#else
	return false;
#endif
}

/*****************************************************************************/
bool MsvcEnvironment::saveMsvcEnvironment()
{
#if defined(CHALET_WIN32)

	auto vcVarsAll = fmt::format("\"{}\\VC\\Auxiliary\\Build\\vcvarsall.bat\"", m_vsAppIdDir);

	// TODO: arch-related stuff
	std::string arch{ "x64" };

	StringList cmd{
		vcVarsAll,
		arch,
		">",
		"nul",
		"&&",
		"SET",
		">",
		m_varsFileMsvc
	};
	bool result = std::system(String::join(cmd).c_str()) == EXIT_SUCCESS;
	return result;
#else
	return false;
#endif
}
}
