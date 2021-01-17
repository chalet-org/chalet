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

/*****************************************************************************/
bool Environment::isBash() noexcept
{
	if (s_terminalType == TerminalType::Unset)
	{
#if defined(CHALET_WIN32)
		// TODO: Check this more thoroughly, but this seems to work
		// auto result = Environment::get("PROMPT");
		// s_terminalType = result == nullptr ? TerminalType::CommandPrompt : TerminalType::Bash;
		s_terminalType = TerminalType::Bash;
#else
		s_terminalType = TerminalType::Bash;
#endif
	}

	return s_terminalType == TerminalType::Bash;
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

	int result = putenv(outValue.c_str());
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
	if (path == nullptr)
	{
		Diagnostic::errorAbort("Could not retrieve PATH");
		return "";
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
		return "";
	}

	std::string ret{ user };
	String::replaceAll(ret, '\\', '/');

	return ret;

#else
	Constant user = Environment::get("HOME");
	if (user == nullptr)
	{
		Diagnostic::errorAbort("Could not resolve user directory");
		return "";
	}

	return std::string(user);
#endif
}
}
