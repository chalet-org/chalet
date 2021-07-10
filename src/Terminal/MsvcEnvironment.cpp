/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Terminal/MsvcEnvironment.hpp"

#include "Core/Arch.hpp"
#include "Core/CommandLineInputs.hpp"
#include "State/BuildState.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Environment.hpp"
#include "Terminal/Output.hpp"
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
MsvcEnvironment::MsvcEnvironment(const CommandLineInputs& inInputs, BuildState& inState) :
	m_inputs(inInputs),
	m_state(inState)
{
#if !defined(CHALET_WIN32)
	UNUSED(m_inputs, m_state);
#endif
}

/*****************************************************************************/
bool MsvcEnvironment::create()
{
#if defined(CHALET_WIN32)
	if (m_initialized)
		return true;

	makeArchitectureCorrections();

	// auto& outputDirectory = m_state.paths.outputDirectory();

	m_varsFileOriginal = m_state.cache.getHashPath("original.env", CacheType::Local);
	m_varsFileMsvc = m_state.cache.getHashPath("msvc_all.env", CacheType::Local);
	m_varsFileMsvcDelta = getMsvcVarsPath();

	m_state.cache.file().addExtraHash(String::getPathFilename(m_varsFileMsvcDelta));

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

	bool deltaExists = Commands::pathExists(m_varsFileMsvcDelta);
	if (!deltaExists)
	{
		// Diagnostic::infoEllipsis("Creating Microsoft{} Visual C++ Environment Cache [{}]", Unicode::registered(), m_varsFileMsvcDelta);
		Diagnostic::infoEllipsis("Creating Microsoft{} Visual C++ Environment Cache", Unicode::registered());

		{
			StringList vswhereCmd{ s_vswhere, "-latest" };
			if (m_inputs.isMsvcPreRelease())
			{
				vswhereCmd.emplace_back("-prerelease");
			}
			vswhereCmd.emplace_back("-property");
			vswhereCmd.emplace_back("installationPath");
			m_vsAppIdDir = Commands::subprocessOutput(vswhereCmd);
		}
		if (m_vsAppIdDir.empty() || !Commands::pathExists(m_vsAppIdDir))
		{
			Diagnostic::error("MSVC Environment could not be fetched: Error running vswhere.exe");
			return false;
		}
		// Read the current environment and save it to a file
		if (!saveOriginalEnvironment())
		{
			Diagnostic::error("MSVC Environment could not be fetched.");
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
					String::replaceAll(line, "\\\\", "\\");
					outContents += line + "\n";
				}
			}
			input.close();
			std::ofstream(m_varsFileMsvcDelta) << outContents;
		}
	}
	else
	{
		// Diagnostic::infoEllipsis("Reading Microsoft{} Visual C++ Environment Cache [{}]", Unicode::registered(), m_varsFileMsvcDelta);
		Diagnostic::infoEllipsis("Reading Microsoft{} Visual C++ Environment Cache", Unicode::registered());
	}

	// Read delta to cache
	{
		std::ifstream input(m_varsFileMsvcDelta);
		for (std::string line; std::getline(input, line);)
		{
			auto splitVar = String::split(line, '=');
			if (splitVar.size() == 2 && splitVar.front().size() > 0 && splitVar.back().size() > 0)
			{
				m_variables[std::move(splitVar.front())] = splitVar.back();
			}
		}
		input.close();
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

	/*auto include = m_variables.find("INCLUDE");
	if (include != m_variables.end())
	{
		String::replaceAll(include->second, "\\", "/");
		m_include = String::split(include->second, ";");
	}*/

	/*auto lib = m_variables.find("LIB");
	if (lib != m_variables.end())
	{
		String::replaceAll(lib->second, "\\", "/");
		m_lib = String::split(lib->second, ";");
	}*/

	/*auto libPath = m_variables.find("LIBPATH");
	if (libPath != m_variables.end())
	{
		String::replaceAll(libPath->second, "\\", "/");
		m_libPath = String::split(libPath->second, ";");
	}*/

	// we got here from "msvc" "msvc-pre" etc from the command line
	if (String::equals({ "msvc", "msvc-pre" }, m_inputs.toolchainPreferenceName()))
	{
		auto cl = Commands::which("cl");
		auto output = Commands::subprocessOutput({ cl });
		auto splitOutput = String::split(output, String::eol());
		for (auto& line : splitOutput)
		{
			if (String::startsWith("Microsoft (R)", line))
			{
				auto pos = line.find("Version");
				if (pos != std::string::npos)
				{
					pos += 8;
					line = line.substr(pos);
					pos = line.find_first_of(" ");
					if (pos != std::string::npos)
					{
						line = line.substr(0, pos);
						if (!line.empty())
						{
							m_inputs.setToolchainPreferenceName(fmt::format("{}-pc-msvc{}", m_inputs.targetArchitecture(), line));
						}
					}
				}
				break;
			}
		}

		auto old = m_varsFileMsvcDelta;
		m_varsFileMsvcDelta = getMsvcVarsPath();
		if (m_varsFileMsvcDelta != old)
		{
			Commands::copyRename(old, m_varsFileMsvcDelta, true);
		}
	}

	m_state.cache.file().addExtraHash(String::getPathFilename(m_varsFileMsvcDelta));

	Diagnostic::printDone(timer.asString());
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
/*const StringList& MsvcEnvironment::libPath() const noexcept
{
	return m_libPath;
}*/

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
	std::string vcvarsFile{ "vcvarsall" };
	const auto& arch = m_state.info.targetArchitectureString();
	StringList allowedArchesWin = Arch::getAllowedMsvcArchitectures();

	if (!String::equals(allowedArchesWin, arch))
	{
		Diagnostic::error("Requested arch '{}' is not supported by {}.bat", arch, vcvarsFile);
		return false;
	}

	// https://docs.microsoft.com/en-us/cpp/build/building-on-the-command-line?view=msvc-160
	auto vcVarsAll = fmt::format("\"{}\\VC\\Auxiliary\\Build\\{}.bat\"", m_vsAppIdDir, vcvarsFile);
	StringList cmd{ vcVarsAll, arch };

	for (auto& arg : m_inputs.archOptions())
	{
		cmd.push_back(arg);
	}

	cmd.emplace_back(">");
	cmd.emplace_back("nul");
	cmd.emplace_back("&&");
	cmd.emplace_back("SET");
	cmd.emplace_back(">");
	cmd.push_back(m_varsFileMsvc);

	bool result = std::system(String::join(cmd).c_str()) == EXIT_SUCCESS;
	return result;
#else
	return false;
#endif
}

/*****************************************************************************/
void MsvcEnvironment::makeArchitectureCorrections()
{
	auto host = m_inputs.hostArchitecture();
	if (String::equals("x86_64", host))
		host = "x64";
	else if (String::equals("i686", host))
		host = "x86";

	bool changed = false;
	bool emptyTarget = m_inputs.targetArchitecture().empty();
	auto arch = m_inputs.targetArchitecture();
	if (emptyTarget)
	{
		if (String::startsWith({ "x64", "x86", "arm" }, m_inputs.toolchainPreferenceName()))
		{
			arch = m_inputs.toolchainPreferenceName().substr(0, 3);
			changed = true;
		}
		else if (String::startsWith("arm64", m_inputs.toolchainPreferenceName()))
		{
			arch = "arm64";
			changed = true;
		}
		else
		{
			arch = m_inputs.hostArchitecture();
			changed = true;
		}
	}
	if (String::equals({ "x86_64", "x64_x64" }, arch))
	{
		arch = "x64";
		changed = true;
	}
	else if (String::equals({ "i686", "x86_x86" }, arch))
	{
		arch = "x86";
		changed = true;
	}
	else if (String::equals("arm64", arch))
	{
		arch = fmt::format("{}_arm64", host);
		changed = true;
	}
	else if (String::equals("arm", arch))
	{
		arch = fmt::format("{}_arm", host);
		changed = true;
	}

	if (changed)
	{
		m_inputs.setTargetArchitecture(std::move(arch));
		m_state.info.setTargetArchitecture(m_inputs.targetArchitecture());
	}
}

/*****************************************************************************/
std::string MsvcEnvironment::getMsvcVarsPath() const
{
	auto archString = m_inputs.getArchWithOptionsAsString(m_state.info.targetArchitectureString());
	archString += fmt::format("_{}", m_inputs.toolchainPreferenceName());
	return m_state.cache.getHashPath(fmt::format("msvc_{}.env", archString), CacheType::Local);
}
}
