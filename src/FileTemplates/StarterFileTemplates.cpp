/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "FileTemplates/StarterFileTemplates.hpp"

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
	const std::string project = inProps.projectName;

	const std::string kind = "consoleApplication";

	const std::string kAbstractsAll = "abstracts:all";
	const std::string kSettingsCxx = "settings:Cxx";
	const std::string kTargets = "targets";
	const std::string kDistribution = "distribution";

	Json ret;
	ret["workspace"] = inProps.workspaceName;
	ret["version"] = inProps.version;

	ret[kAbstractsAll] = Json::object();
	ret[kAbstractsAll]["language"] = language;
	ret[kAbstractsAll][kSettingsCxx] = Json::object();
	ret[kAbstractsAll][kSettingsCxx][langStandardKey] = inProps.langStandard;
	ret[kAbstractsAll][kSettingsCxx]["warnings"] = "pedantic";

	ret[kTargets] = Json::object();
	ret[kTargets][project] = Json::object();
	ret[kTargets][project]["kind"] = kind;
	ret[kTargets][project]["location"] = inProps.location;
	ret[kTargets][project]["runProject"] = true;

	if (!inProps.precompiledHeader.empty())
	{
		ret[kTargets][project][kSettingsCxx] = Json::object();
		ret[kTargets][project][kSettingsCxx]["pch"] = fmt::format("{}/{}", inProps.location, inProps.precompiledHeader);
	}

	ret[kDistribution] = Json::object();
	ret[kDistribution][project] = Json::object();
	ret[kDistribution][project]["projects"] = Json::array();
	ret[kDistribution][project]["projects"][0] = project;

	return ret;
}

/*****************************************************************************/
std::string StarterFileTemplates::getMainCxx(const CodeLanguage inLanguage)
{
	std::string ret;
	if (inLanguage == CodeLanguage::CPlusPlus)
	{
		ret = R"cpp(#include <iostream>

int main(const int argc, const char* const argv[])
{
	std::cout << "Hello world!\n\n";
	std::cout << "Args:\n";

	for (int i=0; i < argc; ++i)
	{
		std::cout << "  " << argv[i] << '\n';
	}

	return 0;
})cpp";
	}
	else
	{
		ret = R"c(#include <stdio.h>

int main(const int argc, const char* const argv[])
{
	printf("Hello, World!\n\n");
	printf("Args:\n");

	for (int i=0; i < argc; ++i)
	{
		printf("%s\n",argv[i]);
	}

	return 0;
})c";
	}

	return ret;
}

/*****************************************************************************/
std::string StarterFileTemplates::getPch(const std::string& inFile, const CodeLanguage inLanguage)
{
	auto file = String::toUpperCase(String::getPathFilename(inFile));
	String::replaceAll(file, '.', '_');
	file.erase(std::remove_if(file.begin(), file.end(), [](char c) {
		return !isalpha(c) && c != '_';
	}),
		file.end());

	std::string ret;
	if (inLanguage == CodeLanguage::CPlusPlus)
	{
		ret = fmt::format(R"cpp(#ifndef {file}
#define {file}

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

#endif // {file})cpp",
			FMT_ARG(file));
	}
	else
	{
		ret = fmt::format(R"c(#ifndef {file}
#define {file}

#include <ctype.h>
#include <float.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <wctype.h>

#endif // {file})c",
			FMT_ARG(file));
	}

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
