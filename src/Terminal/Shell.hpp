/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

namespace chalet
{
struct Shell
{
	Shell() = delete;

	static void detectTerminalType();

	static constexpr bool isSubprocess();
	static constexpr bool isBash();
	static constexpr bool isBashGenericColorTermOrWindowsTerminal();
	static constexpr bool isMicrosoftTerminalOrWindowsBash();
	static constexpr bool isWindowsSubsystemForLinux();
	static constexpr bool isCommandPromptOrPowerShell();
	static constexpr bool isVisualStudioOutput();
	static constexpr bool isJetBrainsOutput();

	static bool isContinuousIntegrationServer();

	static constexpr const char* getNull();

private:
	static void printTermType();

	enum class Type
	{
		Unset,
		Subprocess,
		Bourne, // /bin/sh or /sbin/sh
		Bash,	// /bin/bash
		CShell, // /bin/csh
		TShell, // /bin/tcsh
		Korn,	// /bin/ksh
		Zsh,	// /bin/zsh
		Fish,	// /usr/bin/fish, /usr/local/bin/fish
		WindowsTerminal,
		GenericColorTerm,
		CommandPrompt,
		CommandPromptVisualStudio,
		CommandPromptJetBrains,
		Powershell,
		PowershellIse,
		PowershellOpenSource, // 6+
		PowershellOpenSourceNonWindows,
		WindowsSubsystemForLinux,
	};

	struct State
	{
		Type terminalType = Type::Unset;
		i16 hasTerm = -1;
		i16 isContinuousIntegrationServer = -1;
	};
	static State state;
};
}

#include "Terminal/Shell.inl"
