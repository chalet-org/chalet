/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Terminal/WindowsTerminal.hpp"

#include "Libraries/WindowsApi.hpp"
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
static struct
{
#if defined(CHALET_WIN32)
	uint consoleCp = 0;
	uint consoleOutputCp = 0;
#endif
	bool initialized = false;
} state;
}

/*****************************************************************************/
void WindowsTerminal::initialize()
{
	if (state.initialized)
		return;

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

	// SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS);
	SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
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

	state.initialized = true;
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