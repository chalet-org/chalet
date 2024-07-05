/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Terminal/Shell.hpp"

namespace chalet
{
/*****************************************************************************/
constexpr bool Shell::isSubprocess()
{
	return state.terminalType == Type::Subprocess;
}

/*****************************************************************************/
constexpr bool Shell::isBash()
{
#if defined(CHALET_WIN32)
	return state.terminalType == Type::Bash;
#else
	return state.terminalType != Type::Subprocess && state.terminalType != Type::Unset; // isBash() just looks for a bash-like
#endif
}

/*****************************************************************************/
constexpr bool Shell::isBashGenericColorTermOrWindowsTerminal()
{
#if defined(CHALET_WIN32)
	return state.terminalType == Type::Bash
		|| state.terminalType == Type::GenericColorTerm
		|| state.terminalType == Type::WindowsTerminal;
#else
	return isBash();
#endif
}

/*****************************************************************************/
constexpr bool Shell::isMicrosoftTerminalOrWindowsBash()
{
#if defined(CHALET_WIN32)
	return state.terminalType == Type::CommandPrompt
		|| state.terminalType == Type::CommandPromptVisualStudio
		|| state.terminalType == Type::CommandPromptJetBrains
		|| state.terminalType == Type::Powershell
		|| state.terminalType == Type::PowershellOpenSource
		|| state.terminalType == Type::PowershellIse
		|| state.terminalType == Type::WindowsTerminal
		|| state.terminalType == Type::Bash;
#else
	return false;
#endif
}

/*****************************************************************************/
constexpr bool Shell::isWindowsSubsystemForLinux()
{
#if defined(CHALET_LINUX)
	return state.terminalType == Type::WindowsSubsystemForLinux;
#else
	return false;
#endif
}

/*****************************************************************************/
constexpr bool Shell::isCommandPromptOrPowerShell()
{
	return state.terminalType == Type::CommandPrompt
		|| state.terminalType == Type::CommandPromptVisualStudio
		|| state.terminalType == Type::CommandPromptJetBrains
		|| state.terminalType == Type::Powershell
		|| state.terminalType == Type::PowershellOpenSource
		|| state.terminalType == Type::PowershellIse;
}

/*****************************************************************************/
constexpr bool Shell::isVisualStudioOutput()
{
#if defined(CHALET_WIN32)
	return state.terminalType == Type::CommandPromptVisualStudio;
#else
	return false;
#endif
}

/*****************************************************************************/
constexpr bool Shell::isJetBrainsOutput()
{
#if defined(CHALET_WIN32)
	return state.terminalType == Type::CommandPromptJetBrains;
#else
	return false;
#endif
}

/*****************************************************************************/
constexpr const char* Shell::getNull()
{
#if defined(CHALET_WIN32)
	return "nul";
#else
	return "/dev/null";
#endif
}

}
