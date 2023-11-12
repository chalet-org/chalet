/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Process/Environment.hpp"

#include "Terminal/Commands.hpp"
#include "Utility/Path.hpp"
#include "Utility/String.hpp"

#if defined(CHALET_MSVC)
	#define putenv _putenv
	#pragma warning(push)
	#pragma warning(disable : 4996)
#endif

namespace chalet
{
/*****************************************************************************/
const char* Environment::get(const char* inName)
{
	const char* result = std::getenv(inName);
	return result;
}

/*****************************************************************************/
std::string Environment::getString(const char* inName)
{
	const char* result = std::getenv(inName);
	if (result != nullptr)
		return std::string(result);

	return std::string();
}

/*****************************************************************************/
std::string Environment::getString(const char* inName, const std::string& inFallback)
{
	const char* result = std::getenv(inName);
	if (result != nullptr)
		return std::string(result);

	return inFallback;
}

/*****************************************************************************/
void Environment::set(const char* inName, const std::string& inValue)
{
#if defined(CHALET_WIN32)
	std::string outValue = fmt::format("{}={}", inName, inValue);
	// LOG(outValue);
	i32 result = putenv(outValue.c_str());
#else
	i32 result = 0;
	if (!inValue.empty())
		result = setenv(inName, inValue.c_str(), true);
	else
		unsetenv(inName);
#endif
	if (result != 0)
	{
		Diagnostic::errorAbort("Could not set {}", inName);
	}
}

/*****************************************************************************/
void Environment::replaceCommonVariables(std::string& outString, const std::string& inHomeDirectory)
{
	if (!inHomeDirectory.empty())
	{
		if (String::startsWith("~/", outString))
		{
			outString = fmt::format("{}{}", inHomeDirectory, outString.substr(1));
			Path::toUnix(outString);
		}
	}
}

/*****************************************************************************/
const char* Environment::getPathKey()
{
#if defined(CHALET_WIN32)
	return "Path";
#else
	return "PATH";
#endif
}

/*****************************************************************************/
std::string Environment::getPath()
{
	auto path = Environment::get("PATH");
#if defined(CHALET_WIN32)
	if (path == nullptr)
		path = Environment::get("Path");
#endif
	if (path == nullptr)
	{
		Diagnostic::errorAbort("Could not retrieve {}", Environment::getPathKey());
		return std::string();
	}
	return std::string(path);
}

/*****************************************************************************/
void Environment::setPath(const std::string& inValue)
{
	Environment::set(Environment::getPathKey(), inValue.c_str());
}

/*****************************************************************************/
std::string Environment::getUserDirectory()
{
#if defined(CHALET_WIN32)
	auto user = Environment::get("USERPROFILE");
	if (user == nullptr)
	{
		Diagnostic::errorAbort("Could not resolve user directory");
		return std::string();
	}

	std::string ret{ user };
	String::replaceAll(ret, '\\', '/');

	return ret;

#else
	auto user = Environment::get("HOME");
	if (user == nullptr)
	{
		Diagnostic::errorAbort("Could not resolve user directory");
		return std::string();
	}

	std::string ret(user);
	if (ret.back() == '/')
		ret.pop_back();

	return ret;
#endif
}

/*****************************************************************************/
std::string Environment::getShell()
{
	return getString("SHELL");
}

/*****************************************************************************/
std::string Environment::getComSpec()
{
	return getString("COMSPEC", "cmd.exe");
}

/*****************************************************************************/
bool Environment::saveToEnvFile(const std::string& inOutputFile)
{
#if defined(CHALET_WIN32)
	auto cmdExe = getComSpec();
	StringList cmd{
		std::move(cmdExe),
		"/c",
		"SET"
	};
#else
	auto shell = getShell();
	if (shell.empty())
		return false;

	StringList cmd;
	cmd.emplace_back(std::move(shell));
	cmd.emplace_back("-c");
	cmd.emplace_back("printenv");
#endif
	bool result = Commands::subprocessOutputToFile(cmd, inOutputFile);
	return result;
}

/*****************************************************************************/
void Environment::createDeltaEnvFile(const std::string& inBeforeFile, const std::string& inAfterFile, const std::string& inDeltaFile, const std::function<void(std::string&)>& onReadLine)
{
	if (inBeforeFile.empty() || inAfterFile.empty() || inDeltaFile.empty())
		return;

	{
		std::ifstream afterVars(inAfterFile);
		std::string deltaVars((std::istreambuf_iterator<char>(afterVars)), std::istreambuf_iterator<char>());

		std::ifstream beforeVars(inBeforeFile);
		for (std::string line; std::getline(beforeVars, line);)
		{
			String::replaceAll(deltaVars, line, "");
		}

		std::ofstream(inDeltaFile) << deltaVars;

		afterVars.close();
		beforeVars.close();
	}

	Commands::remove(inBeforeFile);
	Commands::remove(inAfterFile);

	{
		std::string outContents;
		std::ifstream input(inDeltaFile);
		for (std::string line; std::getline(input, line);)
		{
			if (!line.empty())
			{
				onReadLine(line);
				outContents += line + "\n";
			}
		}
		input.close();

		std::ofstream(inDeltaFile) << outContents;
	}
}

/*****************************************************************************/
void Environment::readEnvFileToDictionary(const std::string& inFile, Dictionary<std::string>& outVariables)
{
	std::ifstream input(inFile);
	for (std::string line; std::getline(input, line);)
	{
		auto splitVar = String::split(line, '=');
		if (splitVar.size() == 2 && splitVar.front().size() > 0 && splitVar.back().size() > 0)
		{
			outVariables[std::move(splitVar.front())] = splitVar.back();
		}
	}
	input.close();
}
}

#if defined(CHALET_MSVC)
	#pragma warning(pop)
#endif
