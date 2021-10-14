/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Terminal/Environment.hpp"

#include "Libraries/WindowsApi.hpp"
#include "Terminal/MsvcEnvironment.hpp"
#include "Terminal/Path.hpp"
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

#if defined(CHALET_MSVC)
	#define putenv _putenv
	#pragma warning(push)
	#pragma warning(disable : 4996)
#endif

namespace chalet
{
namespace
{
#if defined(CHALET_WIN32)
DWORD getParentProcessId(DWORD inPid = 0)
{
	HANDLE handle = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (handle != INVALID_HANDLE_VALUE)
	{
		DWORD pid = inPid == 0 ? GetCurrentProcessId() : inPid;
		PROCESSENTRY32 pe;
		pe.dwSize = sizeof(PROCESSENTRY32);

		if (Process32First(handle, &pe))
		{
			do
			{
				if (pe.th32ProcessID == pid)
				{
					return pe.th32ParentProcessID;
				}
			} while (Process32Next(handle, &pe));
		}
	}

	return 0;
}
#else
pid_t getParentProcessId()
{
	return getppid();
}
#endif
#if defined(CHALET_WIN32)
/*****************************************************************************/
std::string getProcessPath(DWORD inPid)
{
	if (inPid != 0)
	{
		HANDLE parentHandle = OpenProcess(
			PROCESS_QUERY_LIMITED_INFORMATION,
			FALSE,
			inPid);

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
	return std::string();
}
#endif

/*****************************************************************************/
#if defined(CHALET_WIN32)
std::pair<std::string, std::string> getParentProcessPaths()
#else
std::string getParentProcessPath()
#endif
{
#if defined(CHALET_WIN32)
	DWORD pid = getParentProcessId();
	std::string parent = getProcessPath(pid);
	DWORD ppid = getParentProcessId(pid);
	std::string pparent = getProcessPath(ppid);
	return std::make_pair<std::string, std::string>(std::move(parent), std::move(pparent));
#else
	// pid_t pid = getpid();
	pid_t pid = getParentProcessId();

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
		name = Commands::subprocessOutput({ "/usr/bin/ls", "-lt", procLoc });
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
bool Environment::isBashGenericColorTermOrWindowsTerminal()
{
#if defined(CHALET_WIN32)
	if (s_terminalType == ShellType::Unset)
		setTerminalType();
	return s_terminalType == ShellType::Bash
		|| s_terminalType == ShellType::GenericColorTerm
		|| s_terminalType == ShellType::WindowsTerminal;
#else
	return isBash();
#endif
}

/*****************************************************************************/
bool Environment::isMicrosoftTerminalOrWindowsBash()
{
	if (s_terminalType == ShellType::Unset)
		setTerminalType();

#if defined(CHALET_WIN32)
	return s_terminalType == ShellType::CommandPrompt
		|| s_terminalType == ShellType::CommandPromptVisualStudio
		|| s_terminalType == ShellType::Powershell
		|| s_terminalType == ShellType::PowershellOpenSource
		|| s_terminalType == ShellType::PowershellIse
		|| s_terminalType == ShellType::WindowsTerminal
		|| s_terminalType == ShellType::Bash;
#else
	return false;
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

	// Powershell needs to be detected from the parent PID
	// Note: env is identical to command prompt. It uses its own env for things like $PSHOME
	{
		const auto& [parentPath, parentParentPath] = getParentProcessPaths();
		if (String::endsWith("WindowsTerminal.exe", parentParentPath))
		{
			s_terminalType = ShellType::WindowsTerminal;
			return printTermType();
		}
		else if (String::endsWith("pwsh.exe", parentPath))
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
		else if (String::endsWith("cmd.exe", parentPath))
		{
			s_terminalType = ShellType::CommandPrompt;
			return printTermType();
		}
	}

	result = Environment::get("COLORTERM");
	if (result != nullptr)
	{
		s_terminalType = ShellType::GenericColorTerm;
		return printTermType();
	}

	// Detect Command prompt from PROMPT
	result = Environment::get("PROMPT");
	if (result != nullptr)
	{
		s_terminalType = ShellType::CommandPrompt;
		return printTermType();
	}
#else
	auto parentPath = getParentProcessPath();
	// LOG("parentPath:", parentPath);

	if (String::endsWith("/bash", parentPath))
	{
		s_terminalType = ShellType::Bash;
		return printTermType();
	}
	else if (String::endsWith("/zsh", parentPath))
	{
		s_terminalType = ShellType::Zsh;
		return printTermType();
	}
	else if (String::endsWith({ "/pwsh", "powershell" }, parentPath))
	{
		s_terminalType = ShellType::PowershellOpenSourceNonWindows;
		return printTermType();
	}
	else if (String::endsWith("/tcsh", parentPath))
	{
		s_terminalType = ShellType::TShell;
		return printTermType();
	}
	else if (String::endsWith("/csh", parentPath))
	{
		s_terminalType = ShellType::CShell;
		return printTermType();
	}
	else if (String::endsWith("/ksh", parentPath))
	{
		s_terminalType = ShellType::Korn;
		return printTermType();
	}
	else if (String::endsWith("/fish", parentPath))
	{
		s_terminalType = ShellType::Fish;
		return printTermType();
	}
	else if (String::endsWith("/sh", parentPath))
	{
		s_terminalType = ShellType::Bourne;
		return printTermType();
	}
#endif

	s_terminalType = ShellType::Subprocess;

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

		case ShellType::WindowsTerminal:
			term = "Windows Terminal (2019)";
			break;

		case ShellType::GenericColorTerm:
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
std::string Environment::getAsString(const char* inName, const std::string& inFallback)
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
	int result = putenv(outValue.c_str());
#else
	int result = setenv(inName, inValue.c_str(), true);
#endif
	if (result != 0)
	{
		Diagnostic::errorAbort("Could not set {}", inName);
	}
}

/*****************************************************************************/
void Environment::replaceCommonVariables(std::string& outString, const std::string& inHomeDirectory)
{
	if (!inHomeDirectory.empty())
	{
		if (String::startsWith("~/", outString))
		{
			outString = fmt::format("{}{}", inHomeDirectory, outString.substr(1));
		}
		else
		{
			String::replaceAll(outString, "${home}", inHomeDirectory);
		}
	}

	if (String::contains("${env:", outString))
	{
		auto begin = outString.find("${env:");
		auto end = outString.find("}", begin);
		if (end != std::string::npos)
		{
			auto replace = outString.substr(begin, (end + 1) - begin);
			auto key = outString.substr(begin + 6, end - (begin + 6));
			auto replaceTo = Environment::getAsString(key.c_str());
			String::replaceAll(outString, replace, replaceTo);
		}
	}

	Path::sanitize(outString);
}

/*****************************************************************************/
const char* Environment::getPathKey()
{
#if defined(CHALET_WIN32)
	return "Path";
#else
	return "PATH";
#endif
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
		Diagnostic::errorAbort("Could not retrieve {}", Environment::getPathKey());
		return std::string();
	}
	return std::string(path);
}

/*****************************************************************************/
void Environment::setPath(const std::string& inValue)
{
	Environment::set(Environment::getPathKey(), inValue.c_str());
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

	std::string ret(user);
	if (ret.back() == '/')
		ret.pop_back();

	return ret;
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
