/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Terminal/Shell.hpp"

#include "Libraries/WindowsApi.hpp"
#include "Process/Environment.hpp"
#include "Terminal/Files.hpp"
#include "Terminal/Output.hpp"
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
	#include <sys/types.h>
	#include <unistd.h>
	#include <array>
#endif

namespace chalet
{
namespace
{
enum class ShellType
{
	Unset,
	Subprocess,
	Bourne, // /bin/sh or /sbin/sh
	Bash,	// /bin/bash
	CShell, // /bin/csh
	TShell, // /bin/tcsh
	Korn,	// /bin/ksh
	Zsh,	// /bin/zsh
	Fish,	// /usr/bin/fish, /usr/local/bin/fish
	WindowsTerminal,
	GenericColorTerm,
	CommandPrompt,
	CommandPromptVisualStudio,
	CommandPromptJetBrains,
	Powershell,
	PowershellIse,
	PowershellOpenSource, // 6+
	PowershellOpenSourceNonWindows,
	WindowsSubsystemForLinux,
};

struct
{
	ShellType terminalType = ShellType::Unset;
	i16 hasTerm = -1;
	i16 isContinuousIntegrationServer = -1;
} state;

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
		std::string procLoc = "/proc/" + std::to_string(static_cast<i32>(pid)) + "/exe";
		name = Files::subprocessOutput({ "/usr/bin/ls", "-lt", procLoc });
		auto list = String::split(name, " -> ");
		name = list.back();
	#endif
	}

	return name;
#endif
}

/*****************************************************************************/
#if defined(CHALET_LINUX)
bool isRunningWindowsSubsystemForLinux()
{
	auto uname = Files::which("uname");
	if (uname.empty())
		return false;

	Output::setShowCommandOverride(false);
	auto result = Files::subprocessOutput({ uname, "-a" });
	Output::setShowCommandOverride(true);
	if (result.empty())
		return false;

	auto lowercase = String::toLowerCase(result);

	return String::contains({ "microsoft", "wsl2" }, lowercase);
}
#endif

/*****************************************************************************/
void printTermType()
{
	std::string term;
	switch (state.terminalType)
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

		case ShellType::CommandPromptJetBrains:
			term = "Command Prompt (CLion / JetBrains)";
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

		case ShellType::WindowsSubsystemForLinux:
			term = "Windows Subsystem for Linux (1 or 2)";
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
void setTerminalType()
{
#if defined(CHALET_WIN32)
	// TOOD: Cygwin, Windows Terminal

	// MSYSTEM: Non-nullptr in MSYS2, Git Bash & std::system calls
	auto result = Environment::get("MSYSTEM");
	if (result != nullptr)
	{
		state.terminalType = ShellType::Bash;
		return printTermType();
	}

	result = Environment::get("VSAPPIDDIR");
	if (result != nullptr)
	{
		state.terminalType = ShellType::CommandPromptVisualStudio;
		return printTermType();
	}

	// Powershell needs to be detected from the parent PID
	// Note: env is identical to command prompt. It uses its own env for things like $PSHOME
	{
		const auto& [parentPath, parentParentPath] = getParentProcessPaths();
		if (String::endsWith("WindowsTerminal.exe", parentParentPath))
		{
			state.terminalType = ShellType::WindowsTerminal;
			return printTermType();
		}
		else if (String::endsWith("pwsh.exe", parentPath))
		{
			state.terminalType = ShellType::PowershellOpenSource;
			return printTermType();
		}
		else if (String::endsWith("powershell_ise.exe", parentPath))
		{
			state.terminalType = ShellType::PowershellIse;
			return printTermType();
		}
		else if (String::endsWith("powershell.exe", parentPath))
		{
			state.terminalType = ShellType::Powershell;
			return printTermType();
		}
		else if (String::endsWith("cmd.exe", parentPath))
		{
			state.terminalType = ShellType::CommandPrompt;
			return printTermType();
		}
		else if (String::endsWith("clion64.exe", parentPath) || String::contains("JetBrains", parentPath))
		{
			state.terminalType = ShellType::CommandPromptJetBrains;
			return printTermType();
		}
	}

	result = Environment::get("COLORTERM");
	if (result != nullptr)
	{
		state.terminalType = ShellType::GenericColorTerm;
		return printTermType();
	}

	// Detect Command prompt from PROMPT
	result = Environment::get("PROMPT");
	if (result != nullptr)
	{
		state.terminalType = ShellType::CommandPrompt;
		return printTermType();
	}
#else
	#if defined(CHALET_LINUX)
	if (isRunningWindowsSubsystemForLinux())
	{
		state.terminalType = ShellType::WindowsSubsystemForLinux;
		return printTermType();
	}
	#endif

	auto parentPath = getParentProcessPath();

	if (String::endsWith("/bash", parentPath))
	{
		state.terminalType = ShellType::Bash;
		return printTermType();
	}
	else if (String::endsWith("/zsh", parentPath))
	{
		state.terminalType = ShellType::Zsh;
		return printTermType();
	}
	else if (String::endsWith(StringList{ "/pwsh", "powershell" }, parentPath))
	{
		state.terminalType = ShellType::PowershellOpenSourceNonWindows;
		return printTermType();
	}
	else if (String::endsWith("/tcsh", parentPath))
	{
		state.terminalType = ShellType::TShell;
		return printTermType();
	}
	else if (String::endsWith("/csh", parentPath))
	{
		state.terminalType = ShellType::CShell;
		return printTermType();
	}
	else if (String::endsWith("/ksh", parentPath))
	{
		state.terminalType = ShellType::Korn;
		return printTermType();
	}
	else if (String::endsWith("/fish", parentPath))
	{
		state.terminalType = ShellType::Fish;
		return printTermType();
	}
	else if (String::endsWith("/sh", parentPath))
	{
		state.terminalType = ShellType::Bourne;
		return printTermType();
	}
#endif

	state.terminalType = ShellType::Subprocess;

	printTermType();
}
}

/*****************************************************************************/
bool Shell::isSubprocess()
{
	if (state.terminalType == ShellType::Unset)
		setTerminalType();

	return state.terminalType == ShellType::Subprocess;
}

/*****************************************************************************/
bool Shell::isBash()
{
	if (state.terminalType == ShellType::Unset)
		setTerminalType();

#if defined(CHALET_WIN32)
	return state.terminalType == ShellType::Bash;
#else
	return state.terminalType != ShellType::Subprocess && state.terminalType != ShellType::Unset; // isBash() just looks for a bash-like
#endif
}

/*****************************************************************************/
bool Shell::isBashGenericColorTermOrWindowsTerminal()
{
#if defined(CHALET_WIN32)
	if (state.terminalType == ShellType::Unset)
		setTerminalType();
	return state.terminalType == ShellType::Bash
		|| state.terminalType == ShellType::GenericColorTerm
		|| state.terminalType == ShellType::WindowsTerminal;
#else
	return isBash();
#endif
}

/*****************************************************************************/
bool Shell::isMicrosoftTerminalOrWindowsBash()
{
	if (state.terminalType == ShellType::Unset)
		setTerminalType();

#if defined(CHALET_WIN32)
	return state.terminalType == ShellType::CommandPrompt
		|| state.terminalType == ShellType::CommandPromptVisualStudio
		|| state.terminalType == ShellType::CommandPromptJetBrains
		|| state.terminalType == ShellType::Powershell
		|| state.terminalType == ShellType::PowershellOpenSource
		|| state.terminalType == ShellType::PowershellIse
		|| state.terminalType == ShellType::WindowsTerminal
		|| state.terminalType == ShellType::Bash;
#else
	return false;
#endif
}

/*****************************************************************************/
bool Shell::isWindowsSubsystemForLinux()
{
	if (state.terminalType == ShellType::Unset)
		setTerminalType();

#if defined(CHALET_LINUX)
	return state.terminalType == ShellType::WindowsSubsystemForLinux;
#else
	return false;
#endif
}

/*****************************************************************************/
bool Shell::isCommandPromptOrPowerShell()
{
	if (state.terminalType == ShellType::Unset)
		setTerminalType();

	return state.terminalType == ShellType::CommandPrompt
		|| state.terminalType == ShellType::CommandPromptVisualStudio
		|| state.terminalType == ShellType::CommandPromptJetBrains
		|| state.terminalType == ShellType::Powershell
		|| state.terminalType == ShellType::PowershellOpenSource
		|| state.terminalType == ShellType::PowershellIse;
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

/*****************************************************************************/
bool Shell::isVisualStudioOutput()
{
	if (state.terminalType == ShellType::Unset)
		setTerminalType();

#if defined(CHALET_WIN32)
	return state.terminalType == ShellType::CommandPromptVisualStudio;
#else
	return false;
#endif
}

/*****************************************************************************/
bool Shell::isJetBrainsOutput()
{
	if (state.terminalType == ShellType::Unset)
		setTerminalType();

#if defined(CHALET_WIN32)
	return state.terminalType == ShellType::CommandPromptJetBrains;
#else
	return false;
#endif
}

/*****************************************************************************/
std::string Shell::getNull()
{
#if defined(CHALET_WIN32)
	return "nul";
#else
	return "/dev/null";
#endif
}

}
