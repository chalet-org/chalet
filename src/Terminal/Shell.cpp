/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Terminal/Shell.hpp"

#include "Libraries/WindowsApi.hpp"
#include "Process/Environment.hpp"
#include "Process/Process.hpp"
#include "System/Files.hpp"
#include "Terminal/Output.hpp"
#include "Utility/String.hpp"
#include "Utility/StringWinApi.hpp"

#if defined(CHALET_WIN32)
	#include <tlhelp32.h>

#elif defined(CHALET_MACOS)
	#include <libproc.h>
	#include <sys/proc_info.h>
	#include <sys/types.h>
	#include <unistd.h>
#else
	#include <sys/types.h>
	#include <unistd.h>
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
			char buffer[1024];
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
std::string getParentProcessPath(DWORD pid)
{
	return getProcessPath(pid);
}
std::string getParentParentProcessPath(DWORD pid)
{
	DWORD ppid = getParentProcessId(pid);
	return getProcessPath(ppid);
}
#else
std::string getParentProcessPath()
{
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
		std::string procLoc = "/proc/" + std::to_string(static_cast<i32>(pid)) + "/exe";
		name = Process::runOutput({ "/usr/bin/ls", "-lt", procLoc });
		auto list = String::split(name, " -> ");
		name = list.back();
	#endif
	}

	return name;
}
#endif

/*****************************************************************************/
#if defined(CHALET_LINUX)
bool isRunningWindowsSubsystemForLinux()
{
	auto uname = Files::which("uname", false);
	if (uname.empty())
		return false;

	Output::setShowCommandOverride(false);
	auto result = Process::runOutput({ uname, "-a" });
	Output::setShowCommandOverride(true);
	if (result.empty())
		return false;

	auto lowercase = String::toLowerCase(result);

	return String::contains({ "microsoft", "wsl2" }, lowercase);
}
#endif
}

/*****************************************************************************/
Shell::State Shell::state;

/*****************************************************************************/
void Shell::printTermType()
{
	std::string term;
	switch (state.terminalType)
	{
		case Type::Bourne:
			term = "Bourne Shell";
			break;

		case Type::Bash:
			term = "Bash";
			break;

		case Type::CShell:
			term = "C Shell";
			break;

		case Type::TShell:
			term = "TENEX C Shell";
			break;

		case Type::Korn:
			term = "Korn Shell";
			break;

		case Type::Zsh:
			term = "Z Shell";
			break;

		case Type::Fish:
			term = "Fish";
			break;

		case Type::Subprocess:
			term = "Subprocess";
			break;

		case Type::WindowsTerminal:
			term = "Windows Terminal (2019)";
			break;

		case Type::GenericColorTerm:
			term = "Generic (w/ COLORTERM set)";
			break;

		case Type::CommandPrompt:
			term = "Command Prompt";
			break;

		case Type::CommandPromptJetBrains:
			term = "Command Prompt (CLion / JetBrains)";
			break;

		case Type::CommandPromptVisualStudio:
			term = "Command Prompt (Visual Studio)";
			break;

		case Type::Powershell:
			term = "Powershell (Windows built-in)";
			break;

		case Type::PowershellIse:
			term = "Powershell ISE";
			break;

		case Type::PowershellOpenSource:
			term = "Powershell (Open Source)";
			break;

		case Type::PowershellOpenSourceNonWindows:
			term = "Powershell (Open Source)";
			break;

		case Type::WindowsSubsystemForLinux:
			term = "Windows Subsystem for Linux (1 or 2)";
			break;

		case Type::Unset:
		default:
			term = "Unset";
			break;
	}

	UNUSED(term);
}
/*****************************************************************************/
void Shell::detectTerminalType()
{
#if defined(CHALET_WIN32)
	// TOOD: Cygwin, Windows Terminal

	auto result = Environment::getString("VSAPPIDDIR");
	if (!result.empty())
	{
		state.terminalType = Type::CommandPromptVisualStudio;
		return printTermType();
	}

	// Jetbrains should set this variable
	DWORD pid = getParentProcessId();
	auto parentPath = getParentProcessPath(pid);
	if (String::endsWith("clion64.exe", parentPath) || String::contains("JetBrains", parentPath))
	{
		state.terminalType = Type::CommandPromptJetBrains;
		return printTermType();
	}

	// MSYSTEM: Non-nullptr in MSYS2, Git Bash & std::system calls
	result = Environment::getString("MSYSTEM");
	if (!result.empty())
	{
		state.terminalType = Type::Bash;
		return printTermType();
	}

	// Note, slower to do the following

	// Powershell needs to be detected from the parent PID
	// Note: env is identical to command prompt. It uses its own env for things like $PSHOME
	{
		auto parentParentPath = getParentParentProcessPath(pid);
		if (String::endsWith("WindowsTerminal.exe", parentParentPath))
		{
			state.terminalType = Type::WindowsTerminal;
			return printTermType();
		}
		else if (String::endsWith("pwsh.exe", parentPath))
		{
			state.terminalType = Type::PowershellOpenSource;
			return printTermType();
		}
		else if (String::endsWith("powershell_ise.exe", parentPath))
		{
			state.terminalType = Type::PowershellIse;
			return printTermType();
		}
		else if (String::endsWith("powershell.exe", parentPath))
		{
			state.terminalType = Type::Powershell;
			return printTermType();
		}
		else if (String::endsWith("cmd.exe", parentPath))
		{
			state.terminalType = Type::CommandPrompt;
			return printTermType();
		}
		else if (String::endsWith("make.exe", parentPath))
		{
			state.terminalType = Type::UnknownOutput;
			return printTermType();
		}
	}

	result = Environment::getString("COLORTERM");
	if (!result.empty())
	{
		state.terminalType = Type::GenericColorTerm;
		return printTermType();
	}

	// Detect Command prompt from PROMPT
	result = Environment::getString("PROMPT");
	if (!result.empty())
	{
		state.terminalType = Type::CommandPrompt;
		return printTermType();
	}
#else
	#if defined(CHALET_LINUX)
	if (isRunningWindowsSubsystemForLinux())
	{
		state.terminalType = Type::WindowsSubsystemForLinux;
		return printTermType();
	}
	#endif

	auto parentPath = getParentProcessPath();

	if (String::endsWith("/bash", parentPath))
	{
		state.terminalType = Type::Bash;
		return printTermType();
	}
	else if (String::endsWith("/zsh", parentPath))
	{
		state.terminalType = Type::Zsh;
		return printTermType();
	}
	else if (String::endsWith(StringList{ "/pwsh", "powershell" }, parentPath))
	{
		state.terminalType = Type::PowershellOpenSourceNonWindows;
		return printTermType();
	}
	else if (String::endsWith("/tcsh", parentPath))
	{
		state.terminalType = Type::TShell;
		return printTermType();
	}
	else if (String::endsWith("/csh", parentPath))
	{
		state.terminalType = Type::CShell;
		return printTermType();
	}
	else if (String::endsWith("/ksh", parentPath))
	{
		state.terminalType = Type::Korn;
		return printTermType();
	}
	else if (String::endsWith("/fish", parentPath))
	{
		state.terminalType = Type::Fish;
		return printTermType();
	}
	else if (String::endsWith("/sh", parentPath))
	{
		state.terminalType = Type::Bourne;
		return printTermType();
	}
	else if (String::endsWith("/make", parentPath))
	{
		// ie. Xcode output
		state.terminalType = Type::UnknownOutput;
		return printTermType();
	}
	#if defined(CHALET_MACOS)
	else if (String::endsWith("/CodeEdit", parentPath))
	{
		state.terminalType = Type::UnknownOutput;
		return printTermType();
	}
	#endif
#endif

	state.terminalType = Type::Subprocess;

	printTermType();
}
/*****************************************************************************/
bool Shell::isContinuousIntegrationServer()
{
	if (state.hasTerm == -1)
	{
		auto varCI = Environment::get("CI");
		state.hasTerm = varCI == nullptr ? 0 : (String::equals(String::toLowerCase(varCI), "true") || String::equals("1", varCI));
	}

	return state.hasTerm == 1;
}
}
