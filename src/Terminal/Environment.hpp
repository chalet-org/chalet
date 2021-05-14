/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_ENVIRONMENT_HPP
#define CHALET_ENVIRONMENT_HPP

namespace chalet
{
struct Environment
{
	static bool isBash();
	static bool isCommandPromptOrPowerShell();
	static bool isMsvc();
	static bool hasTerm();
	static bool isContinuousIntegrationServer();

	static const char* get(const char* inName);
	static void set(const char* inName, const std::string& inValue);

	static std::string getPath();
	static std::string getUserDirectory();

private:
	enum class ShellType
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
		CommandPrompt,
		CommandPromptMsvc,
		Powershell,
		PowershellIse,
		PowershellOpenSource, // 6+
		PowershellOpenSourceNonWindows,
		// WindowsTerminal,
	};

	static void setTerminalType();
	static void printTermType();

	static ShellType s_terminalType;
	static short s_hasTerm;
	static short s_isContinuousIntegrationServer;
};
}

#endif // CHALET_ENVIRONMENT_HPP
