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
	static bool isMsvc();
	static bool hasTerm();

	static const char* get(const char* inName);
	static void set(const char* inName, const std::string& inValue);

	static std::string getPath();
	static std::string getUserDirectory();

private:
	enum class TerminalType
	{
		Unset,
		Bash,
		CommandPrompt,
		CommandPromptMsvc,
		Powershell,
		// WindowsTerminal
	};

	static void setTerminalType();

	static TerminalType s_terminalType;
	static short s_hasTerm;
};
}

#endif // CHALET_ENVIRONMENT_HPP
