/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Terminal/Environment.hpp"

#include "Libraries/Format.hpp"
#include "Libraries/WindowsApi.hpp"
#include "Utility/String.hpp"

#ifdef CHALET_WIN32
	#include <tlhelp32.h>
#endif

namespace chalet
{
namespace
{
/*****************************************************************************/
#if defined(CHALET_WIN32)
std::string getParentProcessPath()
{
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
}
#endif

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

	return s_terminalType == ShellType::Bash;
}

/*****************************************************************************/
bool Environment::isCommandPromptOrPowerShell()
{
	if (s_terminalType == ShellType::Unset)
		setTerminalType();

	return s_terminalType == ShellType::CommandPrompt
		|| s_terminalType == ShellType::Powershell
		|| s_terminalType == ShellType::PowershellOpenSource
		|| s_terminalType == ShellType::PowershellIse;
}

/*****************************************************************************/
bool Environment::isMsvc()
{
	if (s_terminalType == ShellType::Unset)
		setTerminalType();

	return s_terminalType == ShellType::CommandPromptMsvc;
}

/*****************************************************************************/
bool Environment::hasTerm()
{
	if (s_hasTerm == -1)
	{
		auto varTERM = Environment::get("TERM");
		s_hasTerm = varTERM == nullptr ? 0 : 1;
	}

	return s_hasTerm == 1;
}

/*****************************************************************************/
bool Environment::isContinuousIntegrationServer()
{
	if (s_hasTerm == -1)
	{
		auto varCI = Environment::get("CI");
		s_hasTerm = varCI == nullptr ? 0 : (String::equals(String::toLowerCase(varCI), "true") || String::equals(varCI, "1"));
	}

	return s_hasTerm == 1;
}

/*****************************************************************************/
void Environment::setTerminalType()
{
#if defined(CHALET_WIN32)
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
		// LOG("This is running through Visual Studio");
		s_terminalType = ShellType::CommandPromptMsvc;
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

	// TODO: Windows Terminal
#else
	// TODO: Powershell Open Source (barf), zsh, fish distinctions
	s_terminalType = ShellType::Bash;
#endif
	printTermType();
}

/*****************************************************************************/
void Environment::printTermType()
{
	std::string term;
	switch (s_terminalType)
	{
		case ShellType::Bash:
			term = "Bash";
			break;

		case ShellType::Subprocess:
			term = "Subprocess";
			break;

		case ShellType::CommandPrompt:
			term = "Command Prompt";
			break;

		case ShellType::CommandPromptMsvc:
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
std::string Environment::getPath()
{
	auto path = Environment::get("PATH");
#if defined(CHALET_WIN32)
	if (path == nullptr)
		path = Environment::get("Path");
#endif
	if (path == nullptr)
	{
		Diagnostic::errorAbort("Could not retrieve PATH");
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
}
