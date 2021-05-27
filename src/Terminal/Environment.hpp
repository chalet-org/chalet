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
	static bool isBashOrWindowsConPTY();
	static bool isCommandPromptOrPowerShell();
	static bool isVisualStudioCommandPrompt();
	static bool isContinuousIntegrationServer();

	static const char* get(const char* inName);
	static const std::string getAsString(const char* inName, const std::string& inFallback = std::string());
	static void set(const char* inName, const std::string& inValue);

	static bool parseVariablesFromFile(const std::string& inFile);

	static std::string getPath();
	static void setPath(const std::string& inValue);

	static std::string getUserDirectory();
	static std::string getShell();
	static std::string getComSpec();

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
		WindowsConPTY,
		CommandPrompt,
		CommandPromptVisualStudio,
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
