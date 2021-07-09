/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Terminal/OSTerminal.hpp"

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
/*****************************************************************************/
void OSTerminal::initialize()
{
	if (m_initialized)
		return;

#if defined(CHALET_WIN32)
	m_consoleCp = GetConsoleOutputCP();
	m_consoleOutputCp = GetConsoleCP();

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
	{
		// Save the current environment to a file
		// std::system("printenv > all_variables.txt");

		Environment::set("GCC_COLORS", "error=01;31:warning=01;33:note=01;36:caret=01;32:locus=00;34:quote=01");
	}

	reset();

	m_initialized = true;
}

/*****************************************************************************/
void OSTerminal::reset()
{
#if defined(CHALET_WIN32)
	if (!Environment::isBashOrWindowsConPTY() && Output::ansiColorsSupportedInComSpec())
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
void OSTerminal::cleanup()
{
#if defined(CHALET_WIN32)
	if (m_consoleOutputCp != 0)
		SetConsoleOutputCP(m_consoleOutputCp);

	if (m_consoleCp != 0)
		SetConsoleCP(m_consoleCp);
#endif
}
}
