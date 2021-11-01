/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "FileTemplates/StarterFileTemplates.hpp"

#include "State/AncillaryTools.hpp"
#include "State/BuildConfiguration.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Environment.hpp"
#include "Terminal/Path.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"
#include "Json/JsonComments.hpp"

namespace chalet
{
/*****************************************************************************/
Json StarterFileTemplates::getBuildJson(const BuildJsonProps& inProps)
{
	auto getLanguage = [](CxxSpecialization inSpecialization) -> std::string {
		switch (inSpecialization)
		{
			case CxxSpecialization::CPlusPlus: return "C++";
			case CxxSpecialization::ObjectiveC: return "Objective-C";
			case CxxSpecialization::ObjectiveCPlusPlus: return "Objective-C++";
			default: return "C";
		}
	};

	const bool cpp = inProps.language == CodeLanguage::CPlusPlus;
	const bool objectiveCxx = inProps.specialization == CxxSpecialization::ObjectiveC || inProps.specialization == CxxSpecialization::ObjectiveCPlusPlus;
	const std::string language = getLanguage(inProps.specialization);
	const std::string langStandardKey = cpp ? "cppStandard" : "cStandard";
	const std::string project = inProps.projectName;

	const std::string kAbstractsAll = "abstracts:*";
	const std::string kSettingsCxx = "settings:Cxx";
	const std::string kTargets = "targets";
	const std::string kDistribution = "distribution";
	const std::string kConfigurations = "configurations";

	Json ret;
	ret["workspace"] = inProps.workspaceName;
	ret["version"] = inProps.version;

	ret[kAbstractsAll] = Json::object();
	ret[kAbstractsAll]["language"] = language;
	ret[kAbstractsAll][kSettingsCxx] = Json::object();
	ret[kAbstractsAll][kSettingsCxx][langStandardKey] = inProps.langStandard;
	ret[kAbstractsAll][kSettingsCxx]["warnings"] = "pedantic";

	if (inProps.defaultConfigs)
	{
		ret[kConfigurations] = Json::array();
		ret[kConfigurations] = BuildConfiguration::getDefaultBuildConfigurationNames();
	}

	if (objectiveCxx)
	{
		ret[kAbstractsAll][kSettingsCxx]["macosFrameworks"] = Json::array();
		ret[kAbstractsAll][kSettingsCxx]["macosFrameworks"][0] = "Foundation";
	}

	ret[kTargets] = Json::object();
	ret[kTargets][project] = Json::object();
	ret[kTargets][project]["kind"] = "executable";

	if (inProps.useLocation)
	{
		ret[kTargets][project]["location"] = inProps.location;
	}
	else
	{
		ret[kTargets][project]["files"] = Json::array();
		ret[kTargets][project]["files"][0] = fmt::format("{}/{}", inProps.location, inProps.mainSource);
	}

	if (!inProps.precompiledHeader.empty())
	{
		ret[kTargets][project][kSettingsCxx] = Json::object();
		ret[kTargets][project][kSettingsCxx]["pch"] = fmt::format("{}/{}", inProps.location, inProps.precompiledHeader);
	}

	ret[kDistribution] = Json::object();
	ret[kDistribution][project] = Json::object();
	ret[kDistribution][project]["kind"] = "bundle";
	ret[kDistribution][project]["buildTargets"] = Json::array();
	ret[kDistribution][project]["buildTargets"][0] = project;

	return ret;
}

/*****************************************************************************/
std::string StarterFileTemplates::getMainCxx(const CodeLanguage inLanguage, const CxxSpecialization inSpecialization)
{
	std::string ret;
	if (inLanguage == CodeLanguage::CPlusPlus)
	{
		if (inSpecialization == CxxSpecialization::ObjectiveCPlusPlus)
		{
			ret = R"objc(#import <Foundation/Foundation.h>

int main(int argc, const char * argv[])
{
	@autoreleasepool {
		NSLog(@"Hello, World!\n");
		NSLog(@"Args:");

		for (int i=0; i < argc; ++i)
		{
			NSLog(@"%@\n", @(argv[i]));
		}
	}
	return 0;
})objc";
		}
		else
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
	}
	else
	{
		if (inSpecialization == CxxSpecialization::ObjectiveC)
		{
			ret = R"objc(#import <Foundation/Foundation.h>

int main(int argc, const char * argv[])
{
	@autoreleasepool {
		NSLog(@"Hello, World!\n");
		NSLog(@"Args:");

		for (int i=0; i < argc; ++i)
		{
			NSLog(@"%@\n", @(argv[i]));
		}
	}
	return 0;
})objc";
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
	}

	return ret;
}

/*****************************************************************************/
std::string StarterFileTemplates::getPch(const std::string& inFile, const CodeLanguage inLanguage, const CxxSpecialization inSpecialization)
{
	auto file = String::toUpperCase(String::getPathFilename(inFile));
	String::replaceAll(file, '.', '_');
	file.erase(std::remove_if(file.begin(), file.end(), [](char c) {
		return !isalpha(c) && c != '_';
	}),
		file.end());

	UNUSED(inSpecialization);
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
std::string StarterFileTemplates::getGitIgnore(const std::string& inBuildFolder, const std::string& inSettingsFile)
{
	std::string ret = fmt::format(R"(# Chalet
{settings}
{build}
chalet_external
dist

# Other
Thumbs.db
.DS_Store)",
		fmt::arg("settings", inSettingsFile),
		fmt::arg("build", inBuildFolder));

	return ret;
}

/*****************************************************************************/
std::string StarterFileTemplates::getDotEnv()
{
#if defined(CHALET_WIN32)
	std::string ret;
	const auto git = AncillaryTools::getPathToGit();
	auto gitPath = String::getPathFolder(git);
	const bool gitExists = !git.empty();

	auto paths = String::split(Environment::getPath(), ";");
	if (gitExists && !List::contains(paths, gitPath))
	{
		auto programFiles = Environment::getAsString("ProgramFiles");
		Path::sanitize(programFiles);
		String::replaceAll(gitPath, programFiles, "%ProgramFiles%");
		Path::sanitizeForWindows(gitPath);
		ret = fmt::format(R"path(Path={};%Path%)path", gitPath);
	}
	else
	{
		ret = R"(Path=%Path%)";
	}
#else
	std::string ret = R"(PATH=$PATH)";
#endif

	return ret;
}

}
