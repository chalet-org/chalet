/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Terminal/Unicode.hpp"

#include "Terminal/Shell.hpp"

namespace chalet
{
/*****************************************************************************/
inline const char* Unicode::triangle()
{
#if defined(CHALET_WIN32)
	if (Shell::isBasicOutput() || Shell::isJetBrainsOutput())
		return "*";
	else if (Shell::isCommandPromptOrPowerShell())
		return "»";
	else
#endif
		return reinterpret_cast<const char*>(u8"\u25BC"); // triangle
}

/*****************************************************************************/
inline const char* Unicode::diamond()
{
#if defined(CHALET_WIN32)
	if (Shell::isBasicOutput() || Shell::isJetBrainsOutput())
		return "+";
	else if (Shell::isCommandPromptOrPowerShell())
		return "•";
#endif
	return reinterpret_cast<const char*>(u8"\u25C6");
}

/*****************************************************************************/
inline const char* Unicode::checkmark()
{
#if defined(CHALET_WIN32)
	if (Shell::isBasicOutput() || Shell::isJetBrainsOutput())
		return "<";
	else if (Shell::isCommandPromptOrPowerShell())
		return "√";
	else if (Shell::isMicrosoftTerminalOrWindowsBash())
		return reinterpret_cast<const char*>(u8"\u2713");
	else
#elif defined(CHALET_LINUX)
	if (Shell::isWindowsSubsystemForLinux())
		return reinterpret_cast<const char*>(u8"\u2713");
	else
#endif
		return reinterpret_cast<const char*>(u8"\u2714");
}

/*****************************************************************************/
inline const char* Unicode::heavyBallotX()
{
#if defined(CHALET_WIN32)
	if (Shell::isCommandPromptOrPowerShell())
		return "X";
	else
#endif
		return reinterpret_cast<const char*>(u8"\u2718");
}

/*****************************************************************************/
inline const char* Unicode::warning()
{
#if defined(CHALET_WIN32)
	if (Shell::isBasicOutput() || Shell::isJetBrainsOutput())
		return ">";
	else if (Shell::isCommandPromptOrPowerShell())
		return "»";
	else
#endif
		// return reinterpret_cast<const char*>(u8"\u26A0");
		return reinterpret_cast<const char*>(u8"\u25B3");
}

/*****************************************************************************/
inline const char* Unicode::heavyCurvedDownRightArrow()
{
#if defined(CHALET_WIN32)
	if (Shell::isBasicOutput() || Shell::isJetBrainsOutput())
		return "-";
	else if (Shell::isCommandPromptOrPowerShell())
		return "¬";
	else
#endif
		return reinterpret_cast<const char*>(u8"\u27A5");
}

/*****************************************************************************/
inline const char* Unicode::registered()
{
#if defined(CHALET_WIN32)
	if (Shell::isCommandPromptOrPowerShell())
		return " (R)";
	else
#endif
		return reinterpret_cast<const char*>(u8"\u00AE");
}

}
