/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Terminal/WindowsTerminal.hpp"

#include "Libraries/WindowsApi.hpp"
#include "Process/Environment.hpp"
#include "Process/Process.hpp"
#include "System/Files.hpp"
#include "Terminal/Output.hpp"

#if defined(CHALET_WIN32)
	#if !defined(ENABLE_VIRTUAL_TERMINAL_PROCESSING)
		#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
	#endif
	#if !defined(ENABLE_PROCESSED_OUTPUT)
		#define ENABLE_PROCESSED_OUTPUT 0x0001
	#endif
#endif

namespace chalet
{
namespace
{
#if defined(CHALET_WIN32)
struct
{
	DWORD consoleMode = 0;
	u32 consoleCp = 0;
	u32 consoleOutputCp = 0;
	bool firstCall = true;
} state;

BOOL WINAPI ConsoleHandlerRoutine(DWORD dwCtrlType)
{
	if (dwCtrlType == CTRL_C_EVENT)
		return TRUE;

	return FALSE;
}
#endif
}

/*****************************************************************************/
void WindowsTerminal::initialize()
{
#if defined(CHALET_WIN32)
	state.consoleCp = GetConsoleCP();
	state.consoleOutputCp = GetConsoleOutputCP();

	if (state.firstCall)
	{
		auto ansiCodePage = GetACP();
		if (ansiCodePage != CP_UTF8)
		{
			Diagnostic::warn("Many parts of the application will fail if using non-ASCII characters.");
			Diagnostic::warn("Expected the Process code page to be 65001 (UTF-8), but it was: {}", ansiCodePage);
		}
	}

	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	if (hOut != INVALID_HANDLE_VALUE)
	{
		GetConsoleMode(hOut, &state.consoleMode);
	}

	{
		auto result = SetConsoleOutputCP(CP_UTF8); // stdout
		result &= SetConsoleCP(CP_UTF8);		   // stdin

		chalet_assert(result, "Failed to set Console encoding.");
		UNUSED(result);
	}

	{
		// auto result = std::system("cmd -v");
		// LOG("GetConsoleScreenBufferInfo", result);
		// UNUSED(result);
	}

	if (state.firstCall)
	{
		SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS);
		// SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
		// SetPriorityClass(GetCurrentProcess(), NORMAL_PRIORITY_CLASS);
	}

	reset();

	#if defined(CHALET_DEBUG) && defined(CHALET_MSVC)
	if (state.firstCall)
	{
		_CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE | _CRTDBG_MODE_DEBUG);
		_CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);
		_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE | _CRTDBG_MODE_DEBUG);
		_CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDERR);
	}
	#endif

	::SetConsoleCtrlHandler(ConsoleHandlerRoutine, TRUE);

	state.firstCall = false;
#endif
}

/*****************************************************************************/
// A CreateProcess noop prevents misleading benchmarks later on when things need to be measured
// For example, "On My Machine(TM)", CreateProcess takes about 38ms the first time (MSVC Release)
// If compiling with MinGW in Debug mode, it's even slower (1.5s),
// so we'll incur this penalty before we actually make any subsequent CreateProcess calls
//
void WindowsTerminal::initializeCreateProcess()
{
#if defined(CHALET_WIN32)
	auto cmd = Files::which("rundll32");
	Process::runMinimalOutput({ cmd });
#endif
}

/*****************************************************************************/
void WindowsTerminal::reset()
{
#if defined(CHALET_WIN32)
	if (Output::ansiColorsSupportedInComSpec())
	{
		HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
		if (hOut != INVALID_HANDLE_VALUE)
		{
			DWORD dwMode = 0;
			if (GetConsoleMode(hOut, &dwMode))
			{
				dwMode |= (ENABLE_PROCESSED_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
				SetConsoleMode(hOut, dwMode);
			}
		}
	}
#endif
}

/*****************************************************************************/
void WindowsTerminal::cleanup()
{
#if defined(CHALET_WIN32)
	if (state.consoleOutputCp != 0)
		SetConsoleOutputCP(state.consoleOutputCp);

	if (state.consoleCp != 0)
		SetConsoleCP(state.consoleCp);

	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	if (hOut != INVALID_HANDLE_VALUE)
	{
		SetConsoleMode(hOut, state.consoleMode);
	}
#endif
}
}
