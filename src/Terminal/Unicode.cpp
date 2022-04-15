/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Terminal/Unicode.hpp"

#include "Terminal/Environment.hpp"

namespace chalet
{
/*****************************************************************************/
const char* Unicode::triangle()
{
#if defined(CHALET_WIN32)
	if (Environment::isCommandPromptOrPowerShell())
		return "»";
	else
#endif
		return reinterpret_cast<const char*>(u8"\u25BC"); // triangle
}

/*****************************************************************************/
const char* Unicode::diamond()
{
#if defined(CHALET_WIN32)
	if (Environment::isCommandPromptOrPowerShell())
		return "•";
#endif
	return reinterpret_cast<const char*>(u8"\u25C6");
}

/*****************************************************************************/
const char* Unicode::checkmark()
{
#if defined(CHALET_WIN32)
	if (Environment::isCommandPromptOrPowerShell())
		return "√";
	else if (Environment::isMicrosoftTerminalOrWindowsBash())
		return reinterpret_cast<const char*>(u8"\u2713");
	else
#endif
		return reinterpret_cast<const char*>(u8"\u2714");
}

/*****************************************************************************/
const char* Unicode::heavyBallotX()
{
#if defined(CHALET_WIN32)
	if (Environment::isCommandPromptOrPowerShell())
		return "X";
	else
#endif
		return reinterpret_cast<const char*>(u8"\u2718");
}

/*****************************************************************************/
const char* Unicode::warning()
{
#if defined(CHALET_WIN32)
	if (Environment::isCommandPromptOrPowerShell())
		return "»";
	else
#endif
		// return reinterpret_cast<const char*>(u8"\u26A0");
		return reinterpret_cast<const char*>(u8"\u25B3");
}

/*****************************************************************************/
const char* Unicode::circledX()
{
#if defined(CHALET_WIN32)
	if (Environment::isCommandPromptOrPowerShell())
		return "X";
	else if (Environment::isMicrosoftTerminalOrWindowsBash())
		return reinterpret_cast<const char*>(u8"\u2BBE");
	else
#endif
		return reinterpret_cast<const char*>(u8"\u2A02");
}

/*****************************************************************************/
const char* Unicode::heavyCurvedDownRightArrow()
{
#if defined(CHALET_WIN32)
	if (Environment::isCommandPromptOrPowerShell())
		return "¬";
	else
#endif
		return reinterpret_cast<const char*>(u8"\u27A5");
}

/*****************************************************************************/
const char* Unicode::registered()
{
#if defined(CHALET_WIN32)
	if (Environment::isCommandPromptOrPowerShell())
		return " (R)";
	else
#endif
		return reinterpret_cast<const char*>(u8"\u00AE");
}

}
