/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Process/Environment.hpp"

#include "Process/Process.hpp"
#include "System/Files.hpp"
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
	const char* result = ::getenv(inName);
	return result;
}

/*****************************************************************************/
std::string Environment::getString(const char* inName)
{
	const char* result = ::getenv(inName);
	if (result != nullptr)
		return std::string(result);

	return std::string();
}

/*****************************************************************************/
std::string Environment::getString(const char* inName, const std::string& inFallback)
{
	const char* result = ::getenv(inName);
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
	i32 result = ::putenv(outValue.c_str());
#else
	i32 result = 0;
	if (!inValue.empty())
		result = ::setenv(inName, inValue.c_str(), true);
	else
		::unsetenv(inName);
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
const char* Environment::getLibraryPathKey()
{
#if defined(CHALET_LINUX)
	return "LD_LIBRARY_PATH";
#elif defined(CHALET_MACOS)
	return "DYLD_FALLBACK_LIBRARY_PATH";
#else
	return "__CHALET_ERROR_LIBRARY_PATH";
#endif
}

/*****************************************************************************/
const char* Environment::getFrameworkPathKey()
{
#if defined(CHALET_MACOS)
	return "DYLD_FALLBACK_FRAMEWORK_PATH";
#else
	return "__CHALET_ERROR_FRAMEWORK_PATH";
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
std::string Environment::getProgramFiles()
{
	return getString("ProgramFiles");
}
std::string Environment::getProgramFilesX86()
{
	return getString("ProgramFiles(x86)");
}

/*****************************************************************************/
std::string Environment::getChaletParentWorkingDirectory()
{
	return Environment::getString("__CHALET_PARENT_CWD");
}
void Environment::setChaletParentWorkingDirectory(const std::string& inValue)
{
	Environment::set("__CHALET_PARENT_CWD", inValue);
}

/*****************************************************************************/
bool Environment::getChaletTargetFlag()
{
	auto result = Environment::getString("__CHALET_TARGET");
	return !result.empty() && String::equals("1", result);
}
void Environment::setChaletTargetFlag(const bool inValue)
{
	Environment::set("__CHALET_TARGET", inValue ? "1" : std::string());
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
	bool result = Process::runOutputToFile(cmd, inOutputFile);
	return result;
}

/*****************************************************************************/
void Environment::createDeltaEnvFile(const std::string& inBeforeFile, const std::string& inAfterFile, const std::string& inDeltaFile, const std::function<void(std::string&)>& onReadLine)
{
	if (inBeforeFile.empty() || inAfterFile.empty() || inDeltaFile.empty())
		return;

	{
		auto afterVars = Files::ifstream(inAfterFile);
		std::string deltaVars((std::istreambuf_iterator<char>(afterVars)), std::istreambuf_iterator<char>());

		auto beforeVars = Files::ifstream(inBeforeFile);
		std::string line;
		auto lineEnd = beforeVars.widen('\n');
		while (std::getline(beforeVars, line, lineEnd))
		{
			String::replaceAll(deltaVars, line, "");
		}

		Files::ofstream(inDeltaFile) << deltaVars;

		afterVars.close();
		beforeVars.close();
	}

	Files::removeIfExists(inBeforeFile);
	Files::removeIfExists(inAfterFile);

	{
		std::string outContents;
		auto input = Files::ifstream(inDeltaFile);
		std::string line;
		auto lineEnd = input.widen('\n');
		while (std::getline(input, line, lineEnd))
		{
			if (!line.empty())
			{
				onReadLine(line);
				outContents += line + "\n";
			}
		}
		input.close();

		Files::ofstream(inDeltaFile) << outContents;
	}
}

/*****************************************************************************/
void Environment::readEnvFileToDictionary(const std::string& inFile, Dictionary<std::string>& outVariables)
{
	auto input = Files::ifstream(inFile);
	std::string line;
	auto lineEnd = input.widen('\n');
	while (std::getline(input, line, lineEnd))
	{
		auto splitVar = String::split(line, '=');
		if (splitVar.size() == 2 && splitVar.front().size() > 0 && splitVar.back().size() > 0)
		{
			outVariables[std::move(splitVar.front())] = std::move(splitVar.back());
		}
	}
	input.close();
}
}

#if defined(CHALET_MSVC)
	#pragma warning(pop)
#endif
