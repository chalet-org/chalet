/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Core/DotEnvFileParser.hpp"

#include "Compile/Environment/VisualStudioEnvironmentScript.hpp"
#include "Core/CommandLineInputs.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Environment.hpp"
#include "Utility/String.hpp"

#if defined(CHALET_DEBUG)
	#include "Utility/Timer.hpp"
#endif

namespace chalet
{
/*****************************************************************************/
DotEnvFileParser::DotEnvFileParser(const CommandLineInputs& inInputs) :
	m_inputs(inInputs)
{
}

/*****************************************************************************/
bool DotEnvFileParser::serialize()
{
	std::string envFile = searchDotEnv(m_inputs.envFile(), m_inputs.platformEnv());
	if (envFile.empty())
	{
		envFile = searchDotEnv(m_inputs.envFile(), m_inputs.defaultEnvFile());
	}

	if (envFile.empty())
	{
		if (Commands::pathExists(m_inputs.envFile()))
			envFile = m_inputs.envFile();
		else
			return true; // don't care
	}

#if defined(CHALET_DEBUG)
	Timer timer;
#endif
	Diagnostic::infoEllipsis("Reading Environment [{}]", envFile);

	if (!parseVariablesFromFile(envFile))
	{
		Diagnostic::error("There was an error parsing the env file: {}", envFile);
		return false;
	}

#if defined(CHALET_DEBUG)
	Diagnostic::printDone(timer.asString());
#else
	Diagnostic::printDone();
#endif
	return true;
}

/*****************************************************************************/
std::string DotEnvFileParser::searchDotEnv(const std::string& inRelativeEnv, const std::string& inEnv) const
{
	if (String::endsWith(m_inputs.defaultEnvFile(), inRelativeEnv))
	{
		auto toSearch = String::getPathFolder(inRelativeEnv);
		if (!toSearch.empty())
		{
			toSearch = fmt::format("{}/{}", toSearch, inEnv);
			if (Commands::pathExists(toSearch))
				return toSearch;
		}
	}
	else
	{
		if (Commands::pathExists(inRelativeEnv))
			return inRelativeEnv;
	}

	if (Commands::pathExists(inEnv))
		return inEnv;

	return std::string();
}

/*****************************************************************************/
bool DotEnvFileParser::parseVariablesFromFile(const std::string& inFile) const
{
	auto appDataPath = Environment::getAsString("APPDATA");
	StringList pathSearch{ "Path", "PATH" };

#if defined(CHALET_WIN32)
	const bool msvcExists = VisualStudioEnvironmentScript::visualStudioExists();
#endif

	std::ifstream input(inFile);
	for (std::string line; std::getline(input, line);)
	{
		if (line.empty() || String::startsWith('#', line))
			continue;

		if (!String::contains('=', line))
			continue;

		auto splitVar = String::split(line, '=');
		if (splitVar.size() != 2 || splitVar.front().empty())
			continue;

		auto& key = splitVar.front();
		if (String::startsWith(' ', key))
		{
			std::size_t afterSpaces = key.find_first_not_of(' ');
			key = key.substr(afterSpaces);
		}

		auto& value = splitVar.back();
		if (value.empty())
			continue;

#if defined(CHALET_WIN32)
		const bool isPath = String::equals(pathSearch, key);
		for (std::size_t end = value.find_last_of('%'); end != std::string::npos;)
		{
			std::size_t beg = value.substr(0, end).find_last_of('%');
			if (beg == std::string::npos)
				break;

			std::size_t length = (end + 1) - beg;

			std::string capture = value.substr(beg, length);
			std::string replaceKey = value.substr(beg + 1, length - 2);

			// Note: If someone writes "Path=C:\MyPath;%Path%", MSVC Path variables would be placed before C:\MyPath.
			//   This would be a problem is someone is using MinGW and wants to detect the MinGW version of Cmake, Ninja,
			//   or anything else that is also bundled with Visual Studio
			//   To get around this, and have MSVC Path vars before %Path% as expected,
			//   we add a fake path (with valid syntax) to inject it into later (See CompileEnvironmentVisualStudio.cpp)
			//
			auto replaceValue = Environment::getAsString(replaceKey.c_str());
			if (msvcExists && isPath && String::equals(pathSearch, replaceKey))
			{
				value.replace(beg, length, fmt::format("{}\\__CHALET_PATH_INJECT__;{}", appDataPath, replaceValue));
			}
			else
			{
				value.replace(beg, length, replaceValue);
			}

			end = value.find_last_of("%");
		}
#else
		for (std::size_t beg = value.find_last_of('$'); beg != std::string::npos;)
		{
			std::size_t end = value.find_first_of(':', beg);
			if (end == std::string::npos)
				end = value.size();

			std::size_t length = end - beg;

			std::string capture = value.substr(beg, length);
			std::string replaceKey = value.substr(beg + 1, length - 1);

			auto replaceValue = Environment::getAsString(replaceKey.c_str());
			value.replace(beg, length, replaceValue);

			beg = value.find_last_of('$');
		}
#endif

		Environment::set(key.c_str(), value);
	}

	input.close();

	return true;
}

}
