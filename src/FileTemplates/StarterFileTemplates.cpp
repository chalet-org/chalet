/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "FileTemplates/StarterFileTemplates.hpp"

#include "Libraries/Format.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Environment.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"
#include "Json/JsonComments.hpp"

namespace chalet
{
/*****************************************************************************/
Json StarterFileTemplates::getBuildJson(const BuildJsonProps& inProps)
{
	const bool cpp = inProps.language == CodeLanguage::CPlusPlus;
	const std::string language = cpp ? "C++" : "C";
	const std::string langStandardKey = cpp ? "cppStandard" : "cStandard";
	const std::string langStandardValue = cpp ? "c++17" : "c17";

	const std::string kind = "consoleApplication";

	std::string ret = R"json({
	"version": "${version}",
	"workspace": "${workspace}",
	"path": [],
	"abstracts:all": {
		"language": "${language}",
		"settings:Cxx": {
			"${langStandardKey}": "${langStandardValue}",
			"warnings": "pedantic"
		}
	},
	"targets": {
		"${project}": {
			"kind": "${kind}",
			"location": "src",
			"runProject": true,
			"settings:Cxx": {
				"pch": "src/PCH.hpp"
			}
		}
	}
}
)json";

	String::replaceAll(ret, "${version}", inProps.version);
	String::replaceAll(ret, "${workspace}", inProps.workspaceName);
	String::replaceAll(ret, "${project}", inProps.projectName);
	String::replaceAll(ret, "${language}", language);
	String::replaceAll(ret, "${langStandardKey}", langStandardKey);
	String::replaceAll(ret, "${langStandardValue}", langStandardValue);
	String::replaceAll(ret, "${kind}", kind);

	return JsonComments::parseLiteral(ret);
}

/*****************************************************************************/
std::string StarterFileTemplates::getMainCpp()
{
	std::string ret = R"(#include <iostream>

int main(const int argc, const char* const argv[])
{
	std::cout << "Hello world!\n\n";
	std::cout << "Args:\n";

	for (int i=0; i < argc; ++i)
	{
		std::cout << "  " << argv[i] << '\n';
	}

	return 0;
}

)";

	return ret;
}

/*****************************************************************************/
std::string StarterFileTemplates::getPch()
{

	std::string ret = R"(#ifndef PRECOMPILED_HEADER_HPP
#define PRECOMPILED_HEADER_HPP

#include <algorithm>
#include <cstdio>
#include <deque>
#include <fstream>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#endif // PRECOMPILED_HEADER_HPP
)";

	return ret;
}

/*****************************************************************************/
std::string StarterFileTemplates::getGitIgnore(const std::string& inBuildFolder)
{
	std::string ret = fmt::format(R"(# General
Thumbs.db
.DS_Store

# Build
{build}
dist
chalet_external/
)",
		fmt::arg("build", inBuildFolder));

	return ret;
}

/*****************************************************************************/
std::string StarterFileTemplates::getDotEnv()
{
#if defined(CHALET_WIN32)
	std::string ret;
	const auto programFiles = Environment::getAsString("ProgramFiles");
	std::string gitPath = fmt::format("{}/Git/bin", programFiles);
	const bool gitExists = !programFiles.empty() && Commands::pathExists(gitPath);

	auto paths = String::split(Environment::getPath(), ";");
	if (gitExists && !List::contains(paths, gitPath))
	{
		ret = R"(Path=%ProgramFiles%\Git\bin\;%Path%
)";
	}
	else
	{
		ret = R"(Path=%Path%
)";
	}
#else
	std::string ret = R"(PATH=$PATH
)";
#endif

	return ret;
}

}
