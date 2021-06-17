/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Terminal/Unicode.hpp"

#include "Terminal/Environment.hpp"

namespace chalet
{
/*****************************************************************************/
// Looks funky on Mac & Linux with default fonts
const char* Unicode::dot()
{
#if defined(CHALET_WIN32)
	if (Environment::isCommandPromptOrPowerShell() || Environment::isVisualStudioCommandPrompt())
		return "º";
	else
#endif
		return u8"\u25BC"; // triangle
}

/*****************************************************************************/
const char* Unicode::degree()
{
#if defined(CHALET_WIN32)
	if (Environment::isCommandPromptOrPowerShell() || Environment::isVisualStudioCommandPrompt())
		return "º";
	else
#endif
		return u8"\u2218";
}

/*****************************************************************************/
const char* Unicode::triangle()
{
#if defined(CHALET_WIN32)
	if (Environment::isCommandPromptOrPowerShell() || Environment::isVisualStudioCommandPrompt())
		return "»";
	else
#endif
		return u8"\u25BC"; // triangle
}

/*****************************************************************************/
const char* Unicode::diamond()
{
#if defined(CHALET_WIN32)
	if (Environment::isCommandPromptOrPowerShell() || Environment::isVisualStudioCommandPrompt())
		return "º";
	else
#endif
		return u8"\u2666";
}

/*****************************************************************************/
const char* Unicode::heavyCheckmark()
{
#if defined(CHALET_WIN32)
	if (Environment::isCommandPromptOrPowerShell() || Environment::isVisualStudioCommandPrompt())
		return "√";
	else
#endif
		return u8"\u2714";
}

/*****************************************************************************/
const char* Unicode::heavyBallotX()
{
#if defined(CHALET_WIN32)
	if (Environment::isCommandPromptOrPowerShell() || Environment::isVisualStudioCommandPrompt())
		return "X";
	else
#endif
		return u8"\u2718";
}

/*****************************************************************************/
const char* Unicode::boldSaltire()
{
#if defined(CHALET_WIN32)
	if (Environment::isCommandPromptOrPowerShell() || Environment::isVisualStudioCommandPrompt())
		return "X";
	else
#endif
		return u8"\xF0\x9F\x9E\xAB";
}

/*****************************************************************************/
const char* Unicode::warning()
{
#if defined(CHALET_WIN32)
	if (Environment::isCommandPromptOrPowerShell() || Environment::isVisualStudioCommandPrompt())
		return "»";
	else
#endif
		return u8"\u26A0";
}

/*****************************************************************************/
const char* Unicode::circledSaltire()
{
#if defined(CHALET_WIN32)
	if (Environment::isCommandPromptOrPowerShell() || Environment::isVisualStudioCommandPrompt())
		return "X";
	else
#endif
		// return u8"\u2B59";
		return u8"\u2A02";
}

/*****************************************************************************/
const char* Unicode::heavyCurvedDownRightArrow()
{
#if defined(CHALET_WIN32)
	if (Environment::isCommandPromptOrPowerShell() || Environment::isVisualStudioCommandPrompt())
		return "¬";
	else
#endif
		return u8"\u27A5";
}

/*****************************************************************************/
const char* Unicode::heavyCurvedUpRightArrow()
{
#if defined(CHALET_WIN32)
	if (Environment::isCommandPromptOrPowerShell() || Environment::isVisualStudioCommandPrompt())
		return "¬";
	else
#endif
		return u8"\u27A6";
}

/*****************************************************************************/
const char* Unicode::rightwardsTripleArrow()
{
#if defined(CHALET_WIN32)
	if (Environment::isCommandPromptOrPowerShell() || Environment::isVisualStudioCommandPrompt())
		return "=";
	else
#endif
		return u8"\u21DB";
}

/*****************************************************************************/
const char* Unicode::registered()
{
#if defined(CHALET_WIN32)
	if (Environment::isCommandPromptOrPowerShell() || Environment::isVisualStudioCommandPrompt())
		return " (R)";
	else
#endif
		return u8"\u00AE";
}

}
