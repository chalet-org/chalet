/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Terminal/Environment.hpp"

#include "Libraries/Format.hpp"
#include "Libraries/WindowsApi.hpp"
#include "Terminal/MsvcEnvironment.hpp"
#include "Utility/String.hpp"

#if defined(CHALET_WIN32)
	#include <tlhelp32.h>
#elif defined(CHALET_MACOS)
	#include <sys/types.h>
	#include <unistd.h>
	#include <sys/proc_info.h>
	#include <libproc.h>
	#include <array>
#else
	#include "Terminal/Commands.hpp"

	#include <sys/types.h>
	#include <unistd.h>
	#include <array>
#endif

#ifdef CHALET_MSVC
	#pragma warning(push)
	#pragma warning(disable : 4996)
#endif

namespace chalet
{
namespace
{
/*****************************************************************************/
std::string getParentProcessPath()
{
#if defined(CHALET_WIN32)
	HANDLE handle = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (handle)
	{
		DWORD pid = GetCurrentProcessId();
		PROCESSENTRY32 pe;
		pe.dwSize = sizeof(PROCESSENTRY32);

		if (Process32First(handle, &pe))
		{
			do
			{
				if (pe.th32ProcessID == pid)
				{
					HANDLE parentHandle = OpenProcess(
						PROCESS_QUERY_LIMITED_INFORMATION,
						FALSE,
						pe.th32ParentProcessID);

					if (parentHandle)
					{
						std::string processPath;
						DWORD buffSize = 1024;
						CHAR buffer[1024];
						if (QueryFullProcessImageNameA(parentHandle, 0, buffer, &buffSize))
						{
							processPath = std::string(buffer);
						}
						CloseHandle(parentHandle);

						return processPath;
					}
				}
			} while (Process32Next(handle, &pe));
		}
	}
	return std::string();
#else
	// pid_t pid = getpid();
	pid_t pid = getppid();

	std::string name;
	if (pid > 0)
	{
	#if defined(CHALET_MACOS)
		std::array<char, PROC_PIDPATHINFO_MAXSIZE> pathBuffer;
		pathBuffer.fill(0);
		proc_pidpath(pid, pathBuffer.data(), pathBuffer.size());
		if (pathBuffer.size() > 0)
		{
			name = std::string(pathBuffer.data());
		}
	#else
		// TODO: Better solution than this
		std::string procLoc = "/proc/" + std::to_string(static_cast<int>(pid)) + "/exe";
		name = Commands::subprocessOutput({ "ls", "-lt", procLoc }, true);
		auto list = String::split(name, " -> ");
		name = list.back();
	#endif
	}

	return name;
#endif
}
}

/*****************************************************************************/
Environment::ShellType Environment::s_terminalType = ShellType::Unset;
short Environment::s_hasTerm = -1;
short Environment::s_isContinuousIntegrationServer = -1;

/*****************************************************************************/
bool Environment::isBash()
{
	if (s_terminalType == ShellType::Unset)
		setTerminalType();

#if defined(CHALET_WIN32)
	return s_terminalType == ShellType::Bash;
#else
	return s_terminalType != ShellType::Unset; // isBash() just looks for a bash-like
#endif
}

/*****************************************************************************/
bool Environment::isBashOrWindowsConPTY()
{
#if defined(CHALET_WIN32)
	if (s_terminalType == ShellType::Unset)
		setTerminalType();
	return s_terminalType == ShellType::Bash || s_terminalType == ShellType::WindowsConPTY;
#else
	return isBash();
#endif
}

/*****************************************************************************/
bool Environment::isCommandPromptOrPowerShell()
{
	if (s_terminalType == ShellType::Unset)
		setTerminalType();

	// Note: intentionally not using ShellType::CommandPromptVisualStudio
	return s_terminalType == ShellType::CommandPrompt
		|| s_terminalType == ShellType::Powershell
		|| s_terminalType == ShellType::PowershellOpenSource
		|| s_terminalType == ShellType::PowershellIse;
}

/*****************************************************************************/
bool Environment::isVisualStudioCommandPrompt()
{
	if (s_terminalType == ShellType::Unset)
		setTerminalType();

	return s_terminalType == ShellType::CommandPromptVisualStudio;
}

/*****************************************************************************/
bool Environment::isContinuousIntegrationServer()
{
	if (s_hasTerm == -1)
	{
		auto varCI = Environment::get("CI");
		s_hasTerm = varCI == nullptr ? 0 : (String::equals(String::toLowerCase(varCI), "true") || String::equals("1", varCI));
	}

	return s_hasTerm == 1;
}

/*****************************************************************************/
void Environment::setTerminalType()
{
#if defined(CHALET_WIN32)
	// TOOD: Cygwin, Windows Terminal

	// MSYSTEM: Non-nullptr in MSYS2, Git Bash & std::system calls
	auto result = Environment::get("MSYSTEM");
	if (result != nullptr)
	{
		s_terminalType = ShellType::Bash;
		return printTermType();
	}

	result = Environment::get("VSAPPIDDIR");
	if (result != nullptr)
	{
		s_terminalType = ShellType::CommandPromptVisualStudio;
		return printTermType();
	}

	// TODO: Proper check for Windows ConPTY
	//   https://github.com/microsoft/vscode/issues/124427
	result = Environment::get("COLORTERM");
	if (result != nullptr)
	{
		s_terminalType = ShellType::WindowsConPTY;
		return printTermType();
	}

	// Powershell needs to be detected from the parent PID
	// Note: env is identical to command prompt. It uses its own env for things like $PSHOME
	{
		auto parentPath = getParentProcessPath();
		if (String::endsWith("pwsh.exe", parentPath))
		{
			s_terminalType = ShellType::PowershellOpenSource;
			return printTermType();
		}
		else if (String::endsWith("powershell_ise.exe", parentPath))
		{
			s_terminalType = ShellType::PowershellIse;
			return printTermType();
		}
		else if (String::endsWith("powershell.exe", parentPath))
		{
			s_terminalType = ShellType::Powershell;
			return printTermType();
		}
	}

	// Detect Command prompt from PROMPT
	result = Environment::get("PROMPT");
	if (result != nullptr)
	{
		s_terminalType = ShellType::CommandPrompt;
		return printTermType();
	}

	s_terminalType = ShellType::Subprocess;
#else
	auto parentPath = getParentProcessPath();
	// LOG("parentPath:", parentPath);

	if (String::equals("/bin/bash", parentPath))
	{
		s_terminalType = ShellType::Bash;
		return printTermType();
	}
	else if (String::equals({ "/bin/sh", "/sbin/sh" }, parentPath))
	{
		s_terminalType = ShellType::Bourne;
		return printTermType();
	}
	else if (String::equals("/bin/zsh", parentPath))
	{
		s_terminalType = ShellType::Zsh;
		return printTermType();
	}
	else if (String::contains({ "pwsh", "powershell" }, parentPath))
	{
		s_terminalType = ShellType::PowershellOpenSourceNonWindows;
		return printTermType();
	}
	else if (String::equals("/bin/csh", parentPath))
	{
		s_terminalType = ShellType::CShell;
		return printTermType();
	}
	else if (String::equals("/bin/tcsh", parentPath))
	{
		s_terminalType = ShellType::TShell;
		return printTermType();
	}
	else if (String::equals("/bin/ksh", parentPath))
	{
		s_terminalType = ShellType::Korn;
		return printTermType();
	}
	else if (String::endsWith({ "/usr/bin/fish", "/usr/local/bin/fish", "/fish" }, parentPath))
	{
		s_terminalType = ShellType::Fish;
		return printTermType();
	}

	s_terminalType = ShellType::Subprocess;
#endif
	printTermType();
}

/*****************************************************************************/
void Environment::printTermType()
{
	std::string term;
	switch (s_terminalType)
	{
		case ShellType::Bourne:
			term = "Bourne Shell";
			break;

		case ShellType::Bash:
			term = "Bash";
			break;

		case ShellType::CShell:
			term = "C Shell";
			break;

		case ShellType::TShell:
			term = "TENEX C Shell";
			break;

		case ShellType::Korn:
			term = "Korn Shell";
			break;

		case ShellType::Zsh:
			term = "Z Shell";
			break;

		case ShellType::Fish:
			term = "Fish";
			break;

		case ShellType::Subprocess:
			term = "Subprocess";
			break;

		case ShellType::WindowsConPTY:
			term = "Generic (w/ COLORTERM set)";
			break;

		case ShellType::CommandPrompt:
			term = "Command Prompt";
			break;

		case ShellType::CommandPromptVisualStudio:
			term = "Command Prompt (Visual Studio)";
			break;

		case ShellType::Powershell:
			term = "Powershell (Windows built-in)";
			break;

		case ShellType::PowershellIse:
			term = "Powershell ISE";
			break;

		case ShellType::PowershellOpenSource:
			term = "Powershell (Open Source)";
			break;

		case ShellType::PowershellOpenSourceNonWindows:
			term = "Powershell (Open Source)";
			break;

		case ShellType::Unset:
		default:
			term = "Unset";
			break;
	}

	// LOG("Terminal:", term);
	UNUSED(term);
}

/*****************************************************************************/
const char* Environment::get(const char* inName)
{
	const char* result = std::getenv(inName);
	return result;
}

/*****************************************************************************/
const std::string Environment::getAsString(const char* inName, const std::string& inFallback)
{
	const char* result = std::getenv(inName);
	if (result != nullptr)
		return std::string(result);

	return inFallback;
}

/*****************************************************************************/
void Environment::set(const char* inName, const std::string& inValue)
{
#ifdef CHALET_WIN32
	std::string outValue = fmt::format("{}={}", inName, inValue);
	// LOG(outValue);
	#ifdef CHALET_MSVC
	int result = _putenv(outValue.c_str());
	#else
	int result = putenv(outValue.c_str());
	#endif
#else
	int result = setenv(inName, inValue.c_str(), true);
#endif
	if (result != 0)
	{
		Diagnostic::errorAbort(fmt::format("Could not set {}", inName));
	}
}

/*****************************************************************************/
bool Environment::parseVariablesFromFile(const std::string& inFile)
{
	auto appDataPath = Environment::getAsString("APPDATA");
	StringList pathSearch{ "Path", "PATH" };

#if defined(CHALET_WIN32)
	const bool msvcExists = MsvcEnvironment::exists();
#endif

	std::ifstream input(inFile);
	for (std::string line; std::getline(input, line);)
	{
		if (line.empty() || String::startsWith('#', line))
			continue;

		if (!String::contains('=', line))
			continue;

		auto splitVar = String::split(line, '=');
		if (splitVar.size() == 2 && !splitVar.front().empty())
		{
			auto& key = splitVar.front();
			if (String::startsWith(' ', key))
			{
				std::size_t afterSpaces = key.find_first_not_of(' ');
				key = key.substr(afterSpaces);
			}

			const bool isPath = String::equals(pathSearch, key);

			auto& value = splitVar.back();

			if (!value.empty())
			{
#if defined(CHALET_WIN32)
				for (std::size_t end = value.find_last_of('%'); end != std::string::npos;)
				{
					std::size_t beg = value.substr(0, end).find_last_of('%');
					if (beg == std::string::npos)
						break;

					std::size_t length = (end + 1) - beg;

					std::string capture = value.substr(beg, length);
					std::string replaceKey = value.substr(beg + 1, length - 2);

					// Note: If someone writes "Path=C:\MyPath;%Path%", MSVC Path variables would be placed before C:\MyPath.
					//   This would be a problem is someone is using MinGW and wants to detect the MinGW version of Cmake, Ninja,
					//   or anything else that is also bundled with Visual Studio
					//   To get around this, and have MSVC Path vars before %Path% as expected,
					//   we add a fake path (with valid syntax) to inject it into later (See MsvcEnvironment.cpp)
					//
					auto replaceValue = Environment::getAsString(replaceKey.c_str());
					if (msvcExists && isPath && String::equals(pathSearch, replaceKey))
					{
						value.replace(beg, length, fmt::format("{}\\__CHALET_MSVC_INJECT__;{}", appDataPath, replaceValue));
					}
					else
					{
						value.replace(beg, length, replaceValue);
					}

					end = value.find_last_of("%");
				}
#else
				for (std::size_t beg = value.find_last_of('$'); beg != std::string::npos;)
				{
					std::size_t end = value.find_first_of(':', beg);
					if (end == std::string::npos)
						end = value.size();

					std::size_t length = end - beg;

					std::string capture = value.substr(beg, length);
					std::string replaceKey = value.substr(beg + 1, length - 1);

					auto replaceValue = Environment::getAsString(replaceKey.c_str());
					value.replace(beg, length, replaceValue);

					beg = value.find_last_of('$');
				}
#endif

				Environment::set(key.c_str(), value);
			}
		}
	}
	input.close();

	return true;
}

/*****************************************************************************/
std::string Environment::getPath()
{
	auto path = Environment::get("PATH");
#if defined(CHALET_WIN32)
	if (path == nullptr)
		path = Environment::get("Path");
#endif
	if (path == nullptr)
	{
#if defined(CHALET_WIN32)
		Diagnostic::errorAbort("Could not retrieve Path");
#else
		Diagnostic::errorAbort("Could not retrieve PATH");
#endif
		return std::string();
	}
	return std::string(path);
}

/*****************************************************************************/
std::string Environment::getUserDirectory()
{
#ifdef CHALET_WIN32
	auto user = Environment::get("USERPROFILE");
	if (user == nullptr)
	{
		Diagnostic::errorAbort("Could not resolve user directory");
		return std::string();
	}

	std::string ret{ user };
	String::replaceAll(ret, '\\', '/');

	return ret;

#else
	auto user = Environment::get("HOME");
	if (user == nullptr)
	{
		Diagnostic::errorAbort("Could not resolve user directory");
		return std::string();
	}

	return std::string(user);
#endif
}

/*****************************************************************************/
std::string Environment::getShell()
{
	return getAsString("SHELL");
}

/*****************************************************************************/
std::string Environment::getComSpec()
{
	return getAsString("COMSPEC", "cmd.exe");
}
}

#ifdef CHALET_MSVC
	#pragma warning(pop)
#endif
