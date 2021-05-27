/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Terminal/MsvcEnvironment.hpp"

#include "Libraries/Format.hpp"
#include "State/BuildPaths.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Environment.hpp"
#include "Terminal/Unicode.hpp"
#include "Utility/String.hpp"
#include "Utility/Timer.hpp"

namespace chalet
{
#if defined(CHALET_WIN32)
int MsvcEnvironment::s_exists = -1;
std::string MsvcEnvironment::s_vswhere = std::string();
#endif

/*****************************************************************************/
bool MsvcEnvironment::exists()
{
#if defined(CHALET_WIN32)
	if (s_exists == -1)
	{
		// TODO:
		//   Note that if you install vswhere using Chocolatey (instead of the VS/MSBuild installer),
		//   it will be located at %ProgramData%\chocolatey\lib\vswhere\tools\vswhere.exe
		//   https://stackoverflow.com/questions/54305638/how-to-find-vswhere-exe-path

		std::string progFiles = Environment::getAsString("ProgramFiles(x86)");
		s_vswhere = fmt::format("{}\\Microsoft Visual Studio\\Installer\\vswhere.exe", progFiles);

		bool vswhereFound = Commands::pathExists(s_vswhere);
		if (!vswhereFound)
		{
			std::string progFiles64 = Environment::getAsString("ProgramFiles");
			String::replaceAll(s_vswhere, progFiles, progFiles64);

			vswhereFound = Commands::pathExists(s_vswhere);
		}

		s_exists = vswhereFound ? 1 : 0;
	}

	return s_exists == 1;
#else
	return false;
#endif
}

/*****************************************************************************/
MsvcEnvironment::MsvcEnvironment(const CommandLineInputs& inInputs, BuildPaths& inPath) :
	m_inputs(inInputs),
	m_path(inPath)
{
#if !defined(CHALET_WIN32)
	UNUSED(m_inputs, m_path);
#endif
}

/*****************************************************************************/
bool MsvcEnvironment::readCompilerVariables()
{
#if defined(CHALET_WIN32)
	if (m_initialized)
		return true;

	auto& buildPath = m_path.buildPath();

	m_varsFileOriginal = buildPath + "/original.env";
	m_varsFileMsvc = buildPath + "/msvc_all.env";
	m_varsFileMsvcDelta = buildPath + "/msvc.env";

	m_initialized = true;

	// This sets s_vswhere
	if (!MsvcEnvironment::exists())
		return true;

	Timer timer;

	// TODO: Check if Visual Studio is even installed

	auto path = Environment::getPath();

	// Note: See Note about __CHALET_MSVC_INJECT__ in Environment.cpp
	auto appDataPath = Environment::getAsString("APPDATA");
	std::string msvcInject = fmt::format("{}\\__CHALET_MSVC_INJECT__", appDataPath);

	if (!Commands::pathExists(m_varsFileMsvcDelta))
	{
		bool notUsingMsvc = false;
		for (const auto& search : {
				 "clang++",
				 "clang",
				 "g++",
				 "gcc",
				 "c++",
				 "cc",
				 "lld",
				 "ld",
				 "ar",
				 "windres",
			 })
		{
			notUsingMsvc |= !Commands::which(search).empty();
		}
		if (notUsingMsvc)
			return true;

		Diagnostic::info(fmt::format("Creating Microsoft{} Visual C++ Environment Cache [{}]", Unicode::registered(), m_varsFileMsvcDelta), false);

		m_vsAppIdDir = Commands::subprocessOutput({ s_vswhere, "-latest", "-property", "installationPath" });
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
					if (String::startsWith("__VSCMD_PREINIT_PATH=", line))
					{
						if (String::contains(msvcInject, line))
							String::replaceAll(line, msvcInject + ";", "");
					}
					else if (String::startsWith({ "PATH=", "Path=" }, line))
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
	else
	{
		Diagnostic::info(fmt::format("Reading Microsoft{} Visual C++ Environment Cache [{}]", Unicode::registered(), m_varsFileMsvcDelta), false);
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

	StringList pathSearch{ "Path", "PATH" };
	for (auto& [name, var] : m_variables)
	{
		if (String::equals(pathSearch, name))
		{
			if (String::contains(msvcInject, path))
			{
				String::replaceAll(path, msvcInject, var);
				Environment::set(name.c_str(), path);
			}
			else
			{
				Environment::set(name.c_str(), path + ";" + var);
			}
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

	Diagnostic::printDone(timer.asString());
#endif
	return true;
}

/*****************************************************************************/
void MsvcEnvironment::cleanup()
{
#if defined(CHALET_WIN32)
	Commands::remove(m_varsFileMsvcDelta);
#endif
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
	// TODO: MS SDK version, ARM
#if defined(CHALET_WIN32)
	std::string vcvarsFile{ "vcvars64" };
	auto hostArch = m_inputs.targetArchitecture();
	auto targetArch = m_inputs.targetArchitecture();
	if (hostArch == CpuArchitecture::X86 && targetArch == CpuArchitecture::X86)
	{
		vcvarsFile = "vcvars32";
	}
	else if (hostArch == CpuArchitecture::X64 && targetArch == CpuArchitecture::X64)
	{
		vcvarsFile = "vcvars64";
	}
	else if (hostArch == CpuArchitecture::X64 && targetArch == CpuArchitecture::X86)
	{
		vcvarsFile = "vcvarsamd64_x86";
	}
	else if (hostArch == CpuArchitecture::X86 && targetArch == CpuArchitecture::X64)
	{
		vcvarsFile = "vcvarsx86_amd64";
	}
	auto vcVarsAll = fmt::format("\"{}\\VC\\Auxiliary\\Build\\{}.bat\"", m_vsAppIdDir, vcvarsFile);
	StringList cmd{
		vcVarsAll,
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
