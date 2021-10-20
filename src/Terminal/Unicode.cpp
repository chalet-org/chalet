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
		return u8"\u25BC"; // triangle
}

/*****************************************************************************/
const char* Unicode::diamond()
{
#if defined(CHALET_WIN32)
	if (Environment::isCommandPromptOrPowerShell())
		return "•";
	else
#endif
		return u8"\u25C6";
}

/*****************************************************************************/
const char* Unicode::checkmark()
{
#if defined(CHALET_WIN32)
	if (Environment::isCommandPromptOrPowerShell())
		return "√";
	else
#endif
		return u8"\u2713";
}

/*****************************************************************************/
const char* Unicode::heavyBallotX()
{
#if defined(CHALET_WIN32)
	if (Environment::isCommandPromptOrPowerShell())
		return "X";
	else
#endif
		return u8"\u2718";
}

/*****************************************************************************/
const char* Unicode::multiplicationX()
{
#if defined(CHALET_WIN32)
	if (Environment::isCommandPromptOrPowerShell())
		return "X";
	else
#endif
		return u8"\u2715";
}

/*****************************************************************************/
const char* Unicode::warning()
{
#if defined(CHALET_WIN32)
	if (Environment::isCommandPromptOrPowerShell())
		return "»";
	else
#endif
		return u8"\u25B3";
}

/*****************************************************************************/
const char* Unicode::circledX()
{
#if defined(CHALET_WIN32)
	if (Environment::isCommandPromptOrPowerShell())
		return "X";
	else
#endif
		return u8"\u2BBE";
}

/*****************************************************************************/
const char* Unicode::heavyCurvedDownRightArrow()
{
#if defined(CHALET_WIN32)
	if (Environment::isCommandPromptOrPowerShell())
		return "¬";
	else
#endif
		return u8"\u27A5";
}

/*****************************************************************************/
const char* Unicode::heavyCurvedUpRightArrow()
{
#if defined(CHALET_WIN32)
	if (Environment::isCommandPromptOrPowerShell())
		return "¬";
	else
#endif
		return u8"\u27A6";
}

/*****************************************************************************/
const char* Unicode::rightwardsTripleArrow()
{
#if defined(CHALET_WIN32)
	if (Environment::isCommandPromptOrPowerShell())
		return "=";
	else
#endif
		return u8"\u21DB";
}

/*****************************************************************************/
const char* Unicode::registered()
{
#if defined(CHALET_WIN32)
	if (Environment::isCommandPromptOrPowerShell())
		return " (R)";
	else
#endif
		return u8"\u00AE";
}

}
