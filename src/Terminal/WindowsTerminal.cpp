/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Terminal/WindowsTerminal.hpp"

#include "Libraries/WindowsApi.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Environment.hpp"
#include "Terminal/Output.hpp"

#if defined(CHALET_WIN32)
	#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
		#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
	#endif
#endif

namespace chalet
{
namespace
{
#if defined(CHALET_WIN32)
static struct
{
	uint consoleCp = 0;
	uint consoleOutputCp = 0;
} state;
#endif
}

/*****************************************************************************/
void WindowsTerminal::initialize()
{
#if defined(CHALET_WIN32)
	state.consoleCp = GetConsoleOutputCP();
	state.consoleOutputCp = GetConsoleCP();

	{
		auto result = SetConsoleOutputCP(CP_UTF8);
		result &= SetConsoleCP(CP_UTF8);
		chalet_assert(result, "Failed to set Console encoding.");
		UNUSED(result);
	}

	{
		// auto result = std::system("cmd -v");
		// LOG("GetConsoleScreenBufferInfo", result);
		// UNUSED(result);
	}

	SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS);
	// SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
	// SetPriorityClass(GetCurrentProcess(), NORMAL_PRIORITY_CLASS);

#endif

	reset();

#if defined(CHALET_DEBUG) && defined(CHALET_MSVC)
	{
		_CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE | _CRTDBG_MODE_DEBUG);
		_CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);
		_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE | _CRTDBG_MODE_DEBUG);
		_CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDERR);
	}
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
	auto cmd = Commands::which("rundll32");
	Commands::subprocessNoOutput({ cmd });
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
				dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
				dwMode |= DISABLE_NEWLINE_AUTO_RETURN;
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
#endif
}
}
