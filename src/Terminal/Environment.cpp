/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Terminal/Environment.hpp"

#include "Libraries/Format.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
Environment::TerminalType Environment::s_terminalType = TerminalType::Unset;
short Environment::s_hasTerm = -1;

/*****************************************************************************/
bool Environment::isBash()
{
	if (s_terminalType == TerminalType::Unset)
		setTerminalType();

	return s_terminalType == TerminalType::Bash;
}

/*****************************************************************************/
bool Environment::isMsvc()
{
	if (s_terminalType == TerminalType::Unset)
		setTerminalType();

	return s_terminalType == TerminalType::CommandPromptMsvc;
}

/*****************************************************************************/
bool Environment::hasTerm()
{
	if (s_hasTerm == -1)
	{
		auto varTERM = Environment::get("TERM");
		s_hasTerm = varTERM == nullptr ? 0 : 1;
	}

	return s_hasTerm == 1;
}

/*****************************************************************************/
void Environment::setTerminalType()
{
#if defined(CHALET_WIN32)
	// MSYSTEM: Non-nullptr in MSYS2, Git Bash & std::system calls
	auto result = Environment::get("MSYSTEM");
	if (result != nullptr)
	{
		s_terminalType = TerminalType::Bash;
		return;
	}

	result = Environment::get("VSAPPIDDIR");
	if (result != nullptr)
	{
		LOG("This is running through Visual Studio");
		s_terminalType = TerminalType::CommandPromptMsvc;
		return;
	}

	// PROMPT: nullptr in MSYS2/Git Bash, but not in std::system calls
	result = Environment::get("PROMPT"); // Command Prompt
	if (result != nullptr)
	{
		s_terminalType = TerminalType::CommandPrompt;
		return;
	}

	// Assume it's Powershell. PSHOME will be defined if it is, but it won't technically be part of "Env:"
	// aka 'echo $PSHOME' (PS path) vs 'echo $env:PSHOME' (blank) -- JOY
	s_terminalType = TerminalType::Powershell;

	// TODO: Windows Terminal
#else
	s_terminalType = TerminalType::Bash;
#endif
}

/*****************************************************************************/
Constant Environment::get(const char* inName)
{
	Constant result = std::getenv(inName);
	return result;
}

/*****************************************************************************/
void Environment::set(const char* inName, const std::string& inValue)
{
#ifdef CHALET_WIN32
	std::string outValue = fmt::format("{}={}", inName, inValue);
	// LOG(outValue);
	#ifdef CHALET_MSVC
	int result = _putenv(outValue.c_str());
	#else
	int result = putenv(outValue.c_str());
	#endif
#else
	int result = setenv(inName, inValue.c_str(), true);
#endif
	if (result != 0)
	{
		Diagnostic::errorAbort(fmt::format("Could not set {}", inName));
	}
}

/*****************************************************************************/
std::string Environment::getPath()
{
	Constant path = Environment::get("PATH");
#if defined(CHALET_WIN32)
	if (path == nullptr)
		path = Environment::get("Path");
#endif
	if (path == nullptr)
	{
		Diagnostic::errorAbort("Could not retrieve PATH");
		return std::string();
	}
	return std::string(path);
}

/*****************************************************************************/
std::string Environment::getUserDirectory()
{
#ifdef CHALET_WIN32
	Constant user = Environment::get("USERPROFILE");
	if (user == nullptr)
	{
		Diagnostic::errorAbort("Could not resolve user directory");
		return std::string();
	}

	std::string ret{ user };
	String::replaceAll(ret, '\\', '/');

	return ret;

#else
	Constant user = Environment::get("HOME");
	if (user == nullptr)
	{
		Diagnostic::errorAbort("Could not resolve user directory");
		return std::string();
	}

	return std::string(user);
#endif
}
}
