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
Json StarterFileTemplates::getStandardChaletJson(const BuildJsonProps& inProps)
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
	const auto& project = inProps.projectName;

	std::string langStandard;
	if (inProps.langStandard.size() <= 2)
	{
		langStandard = cpp ? fmt::format("c++{}", inProps.langStandard) : fmt::format("c{}", inProps.langStandard);
	}

	const std::string kAbstractsAll = "abstracts:*";
	const std::string kSettingsCxx = "settings:Cxx";
	const std::string kTargets = "targets";
	const std::string kDistribution = "distribution";
	const std::string kDefaultConfigurations = "defaultConfigurations";

	Json ret;
	ret["workspace"] = inProps.workspaceName;
	ret["version"] = inProps.version;

	if (inProps.defaultConfigs)
	{
		ret[kDefaultConfigurations] = Json::array();
		ret[kDefaultConfigurations] = BuildConfiguration::getDefaultBuildConfigurationNames();
	}

	ret[kAbstractsAll] = Json::object();
	ret[kAbstractsAll]["language"] = language;
	ret[kAbstractsAll][kSettingsCxx] = Json::object();
	ret[kAbstractsAll][kSettingsCxx][langStandardKey] = std::move(langStandard);
	ret[kAbstractsAll][kSettingsCxx]["warnings"] = "pedantic";

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

	if (inProps.modules)
	{
		if (!ret[kTargets][project][kSettingsCxx].is_object())
			ret[kTargets][project][kSettingsCxx] = Json::object();

		ret[kTargets][project][kSettingsCxx]["cppModules"] = true;
	}

	ret[kDistribution] = Json::object();
	ret[kDistribution][project] = Json::object();
	ret[kDistribution][project]["kind"] = "bundle";
	ret[kDistribution][project]["buildTargets"] = Json::array();
	ret[kDistribution][project]["buildTargets"][0] = project;

	return ret;
}

/*****************************************************************************/
std::string StarterFileTemplates::getMainCxx(const CxxSpecialization inSpecialization, const bool inModules)
{
	std::string ret;
	if (inSpecialization == CxxSpecialization::CPlusPlus)
	{
		if (inModules)
		{
			ret = R"cpp(import <iostream>;

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
	else if (inSpecialization == CxxSpecialization::C)
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
	else if (inSpecialization == CxxSpecialization::ObjectiveCPlusPlus)
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

	else if (inSpecialization == CxxSpecialization::ObjectiveC)
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

	auto paths = String::split(Environment::getPath(), Environment::getPathSeparator());
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

/*****************************************************************************/
Json StarterFileTemplates::getCMakeStarterChaletJson(const BuildJsonProps& inProps)
{
	const std::string kTargets = "targets";
	const std::string kDistribution = "distribution";
	const std::string kDefaultConfigurations = "defaultConfigurations";
	const auto& project = inProps.projectName;

	Json ret;
	ret["workspace"] = inProps.workspaceName;
	ret["version"] = inProps.version;

	ret[kDefaultConfigurations] = Json::array();
	ret[kDefaultConfigurations] = {
		"Release",
		"Debug",
		"MinSizeRel",
		"RelWithDebInfo",
	};

	ret[kTargets] = Json::object();
	ret[kTargets][project] = Json::object();
	ret[kTargets][project]["kind"] = "cmakeProject";
	ret[kTargets][project]["location"] = ".";
	ret[kTargets][project]["recheck"] = true;
	ret[kTargets][project]["runExecutable"] = project;

	ret[kDistribution] = Json::object();
	ret[kDistribution][project] = Json::object();
	ret[kDistribution][project]["kind"] = "bundle";
	ret[kDistribution][project]["include"] = Json::array();
	ret[kDistribution][project]["include"][0] = fmt::format("${{buildDir}}/{}", project);

	return ret;
}

/*****************************************************************************/
std::string StarterFileTemplates::getCMakeStarter(const BuildJsonProps& inProps)
{
	UNUSED(inProps);
	const auto& version = inProps.version;
	const auto& workspaceName = inProps.workspaceName;
	const auto& projectName = inProps.projectName;
	const auto& location = inProps.location;
	auto main = fmt::format("{}/{}", location, inProps.mainSource);
	auto pch = fmt::format("{}/{}", location, inProps.precompiledHeader);
	auto sourceExt = String::getPathSuffix(inProps.mainSource);

	std::string minimumCMakeVersion{ "3.12" };
	std::string precompiledHeader;
	if (!inProps.precompiledHeader.empty())
	{
		minimumCMakeVersion = "3.16";
		precompiledHeader = fmt::format(R"cmake(
target_precompile_headers(${{TARGET_NAME}} PRIVATE {pch}))cmake",
			FMT_ARG(pch));
	}

	std::string standard;
	std::string standardRequired;
	std::string extraSettings;
	std::string extraProperties;
	if (inProps.specialization == CxxSpecialization::C)
	{
		if (String::equals({ "17", "23" }, inProps.langStandard))
			minimumCMakeVersion = "3.21";

		standard = fmt::format("CMAKE_C_STANDARD {}", inProps.langStandard);
		standardRequired = "CMAKE_C_STANDARD_REQUIRED";
	}
#if defined(CHALET_MACOS)
	else if (inProps.specialization == CxxSpecialization::ObjectiveCPlusPlus)
	{
		standard = fmt::format("CMAKE_OBJCXX_STANDARD {}", inProps.langStandard);
		standardRequired = "CMAKE_OBJCXX_STANDARD_REQUIRED";
		extraSettings = R"cmake(
enable_language(OBJCXX))cmake";
		extraProperties = R"cmake(
target_link_libraries(${TARGET_NAME} PRIVATE "-framework Foundation")
)cmake";
	}
	else if (inProps.specialization == CxxSpecialization::ObjectiveC)
	{
		standard = fmt::format("CMAKE_OBJC_STANDARD {}", inProps.langStandard);
		standardRequired = "CMAKE_OBJC_STANDARD_REQUIRED";
		extraSettings = R"cmake(
enable_language(OBJC))cmake";
		extraProperties = R"cmake(
target_link_libraries(${TARGET_NAME} PRIVATE "-framework Foundation")
)cmake";
	}
#endif
	else
	{
		if (String::equals("23", inProps.langStandard))
			minimumCMakeVersion = "3.20";

		standard = fmt::format("CMAKE_CXX_STANDARD {}", inProps.langStandard);
		standardRequired = "CMAKE_CXX_STANDARD_REQUIRED";
	}

	std::string sources;
	if (inProps.useLocation)
	{
		sources = fmt::format(R"cmake(file(GLOB_RECURSE SOURCES {location}/*.{sourceExt}))cmake",
			FMT_ARG(location),
			FMT_ARG(sourceExt));
	}
	else
	{
		sources = fmt::format(R"cmake(set(SOURCES {main}))cmake",
			FMT_ARG(main));
	}

	std::string ret = fmt::format(R"cmake(cmake_minimum_required(VERSION {minimumCMakeVersion})

project({workspaceName} VERSION {version})

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set({standard})
set({standardRequired} ON){extraSettings}

set(TARGET_NAME {projectName})
{sources}

add_executable(${{TARGET_NAME}} ${{SOURCES}})
{precompiledHeader}
target_include_directories(${{TARGET_NAME}} PRIVATE {location}/){extraProperties})cmake",
		FMT_ARG(minimumCMakeVersion),
		FMT_ARG(workspaceName),
		FMT_ARG(version),
		FMT_ARG(standard),
		FMT_ARG(standardRequired),
		FMT_ARG(extraSettings),
		FMT_ARG(projectName),
		FMT_ARG(sources),
		FMT_ARG(precompiledHeader),
		FMT_ARG(location),
		FMT_ARG(extraProperties));

	return ret;
}
}
