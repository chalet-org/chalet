/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "FileTemplates/StarterFileTemplates.hpp"

#include "Process/Environment.hpp"
#include "State/AncillaryTools.hpp"
#include "State/BuildConfiguration.hpp"
#include "System/Files.hpp"
#include "Utility/List.hpp"
#include "Utility/Path.hpp"
#include "Utility/String.hpp"
#include "Json/JsonComments.hpp"
#include "Json/JsonKeys.hpp"
#include "Json/JsonValues.hpp"

namespace chalet
{
/*****************************************************************************/
Json StarterFileTemplates::getStandardChaletJson(const ChaletJsonProps& inProps)
{
	auto getLanguage = [](CodeLanguage inLanguage) -> const char* {
		switch (inLanguage)
		{
			case CodeLanguage::CPlusPlus: return "C++";
			case CodeLanguage::ObjectiveC: return "Objective-C";
			case CodeLanguage::ObjectiveCPlusPlus: return "Objective-C++";
			default: return "C";
		}
	};

	const bool cpp = inProps.language == CodeLanguage::CPlusPlus || inProps.language == CodeLanguage::ObjectiveCPlusPlus;
	const bool objectiveCxx = inProps.language == CodeLanguage::ObjectiveC || inProps.language == CodeLanguage::ObjectiveCPlusPlus;
	auto language = getLanguage(inProps.language);
	auto langStandardKey = cpp ? "cppStandard" : "cStandard";
	const auto& project = inProps.projectName;

	std::string langStandard;
	if (inProps.langStandard.size() <= 2)
	{
		langStandard = cpp ? fmt::format("c++{}", inProps.langStandard) : fmt::format("c{}", inProps.langStandard);
	}

	Json ret;
	ret[Keys::WorkspaceName] = inProps.workspaceName;
	ret[Keys::WorkspaceVersion] = inProps.version;

	if (inProps.defaultConfigs)
	{
		ret[Keys::DefaultConfigurations] = Json::array();
		ret[Keys::DefaultConfigurations] = BuildConfiguration::getDefaultBuildConfigurationNames();
	}

	ret[Keys::Targets] = Json::object();
	ret[Keys::Targets][project] = Json::object();
	ret[Keys::Targets][project][Keys::Kind] = "executable";
	ret[Keys::Targets][project]["language"] = language;

	ret[Keys::Targets][project][Keys::SettingsCxx] = Json::object();
	ret[Keys::Targets][project][Keys::SettingsCxx][langStandardKey] = std::move(langStandard);
	if (inProps.modules)
	{
		ret[Keys::Targets][project][Keys::SettingsCxx]["cppModules"] = true;
	}
	ret[Keys::Targets][project][Keys::SettingsCxx]["warningsPreset"] = "pedantic";
	if (!inProps.precompiledHeader.empty())
	{
		ret[Keys::Targets][project][Keys::SettingsCxx]["precompiledHeader"] = fmt::format("{}/{}", inProps.location, inProps.precompiledHeader);
	}
	ret[Keys::Targets][project][Keys::SettingsCxx]["includeDirs"] = Json::array();
	ret[Keys::Targets][project][Keys::SettingsCxx]["includeDirs"][0] = inProps.location;

	if (objectiveCxx)
	{
		ret[Keys::Targets][project][Keys::SettingsCxx]["appleFrameworks"] = Json::array();
		ret[Keys::Targets][project][Keys::SettingsCxx]["appleFrameworks"][0] = "Foundation";
	}
	if (inProps.useLocation)
	{
		auto extension = String::getPathSuffix(inProps.mainSource);
		ret[Keys::Targets][project]["files"] = fmt::format("{}/**.{}", inProps.location, extension);
	}
	else
	{
		ret[Keys::Targets][project]["files"] = Json::array();
		ret[Keys::Targets][project]["files"][0] = fmt::format("{}/{}", inProps.location, inProps.mainSource);
	}

	std::string distTarget = Values::All;
	ret[Keys::Distribution] = Json::object();
	ret[Keys::Distribution][distTarget] = Json::object();
	ret[Keys::Distribution][distTarget][Keys::Kind] = "bundle";
	ret[Keys::Distribution][distTarget]["buildTargets"] = "*";

	return ret;
}

/*****************************************************************************/
std::string StarterFileTemplates::getMainCxx(const CodeLanguage inLanguage, const std::string& inStandard, const bool inModules)
{
	std::string ret;
	if (inLanguage == CodeLanguage::CPlusPlus)
	{
		if (inModules && String::equals("20", inStandard))
		{
			ret = R"cpp(import <iostream>;

int main(const int argc, const char* argv[])
{
	std::cout << "Hello world!\n\n";
	std::cout << "Args:\n";

	for (int i = 0; i < argc; ++i)
	{
		std::cout << "  " << argv[i] << '\n';
	}

	return 0;
})cpp";
		}
		else if (inModules)
		{
			// c++ 23 and up
			ret = R"cpp(import std;

int main(const int argc, const char* argv[])
{
	std::cout << "Hello world!\n\n";
	std::cout << "Args:\n";

	for (int i = 0; i < argc; ++i)
	{
		std::cout << "  " << argv[i] << '\n';
	}

	return 0;
})cpp";
		}
		else
		{
			ret = R"cpp(#include <iostream>

int main(const int argc, const char* argv[])
{
	std::cout << "Hello world!\n\n";
	std::cout << "Args:\n";

	for (int i = 0; i < argc; ++i)
	{
		std::cout << "  " << argv[i] << '\n';
	}

	return 0;
})cpp";
		}
	}
	else if (inLanguage == CodeLanguage::C)
	{

		ret = R"c(#include <stdio.h>

int main(const int argc, const char* argv[])
{
	printf("Hello, World!\n\n");
	printf("Args:\n");

	for (int i = 0; i < argc; ++i)
	{
		printf("%s\n",argv[i]);
	}

	return 0;
})c";
	}
	else if (inLanguage == CodeLanguage::ObjectiveCPlusPlus)
	{
		ret = R"objc(#import <Foundation/Foundation.h>

int main(int argc, const char* argv[])
{
	@autoreleasepool {
		NSLog(@"Hello, World!\n");
		NSLog(@"Args:");

		for (int i = 0; i < argc; ++i)
		{
			NSLog(@"%@\n", @(argv[i]));
		}
	}
	return 0;
})objc";
	}

	else if (inLanguage == CodeLanguage::ObjectiveC)
	{
		ret = R"objc(#import <Foundation/Foundation.h>

int main(int argc, const char* argv[])
{
	@autoreleasepool {
		NSLog(@"Hello, World!\n");
		NSLog(@"Args:");

		for (int i = 0; i < argc; ++i)
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
std::string StarterFileTemplates::getPch(const std::string& inFile, const CodeLanguage inLanguage)
{
	auto file = String::toUpperCase(String::getPathFilename(inFile));
	file.erase(std::remove_if(file.begin(), file.end(), [](char c) {
		return !isalpha(c) && c != '.';
	}),
		file.end());

	auto baseName = String::getPathBaseName(file);
	if (baseName.empty())
	{
		auto suffix = String::getPathSuffix(file);
		file = fmt::format("PCH_{}", suffix);
	}
	else
	{
		String::replaceAll(file, '.', '_');
	}

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
{build}/
chalet_external/
dist/
{settings}
.env*

# Editors
.idea/
.vs/
.vscode/
.fleet/

# Other
.cache/
Thumbs.db
.DS_Store)",
		fmt::arg("build", inBuildFolder),
		fmt::arg("settings", inSettingsFile));

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
		auto programFiles = Environment::getProgramFiles();
		Path::toUnix(programFiles);
		String::replaceAll(gitPath, programFiles, "%ProgramFiles%");
		Path::toWindows(gitPath);
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
Json StarterFileTemplates::getCMakeStarterChaletJson(const ChaletJsonProps& inProps)
{
	const auto& project = inProps.projectName;

	Json ret;
	ret[Keys::WorkspaceName] = inProps.workspaceName;
	ret[Keys::WorkspaceVersion] = inProps.version;

	ret[Keys::DefaultConfigurations] = Json::array();
	ret[Keys::DefaultConfigurations] = {
		"Release",
		"Debug",
		"MinSizeRel",
		"RelWithDebInfo",
	};

	ret[Keys::Targets] = Json::object();
	ret[Keys::Targets][project] = Json::object();
	ret[Keys::Targets][project][Keys::Kind] = "cmakeProject";
	ret[Keys::Targets][project]["location"] = ".";
	ret[Keys::Targets][project]["recheck"] = true;
	ret[Keys::Targets][project]["runExecutable"] = project;

	ret[Keys::Distribution] = Json::object();
	ret[Keys::Distribution][project] = Json::object();
	ret[Keys::Distribution][project][Keys::Kind] = "bundle";
	ret[Keys::Distribution][project]["include"] = Json::array();
	ret[Keys::Distribution][project]["include"][0] = fmt::format("${{externalBuild:{}}}/${{exe:{}}}", project, project);

	return ret;
}

/*****************************************************************************/
std::string StarterFileTemplates::getCMakeStarter(const ChaletJsonProps& inProps)
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
	if (inProps.language == CodeLanguage::C)
	{
		if (String::equals(StringList{ "17", "23" }, inProps.langStandard))
			minimumCMakeVersion = "3.21";

		standard = fmt::format("CMAKE_C_STANDARD {}", inProps.langStandard);
		standardRequired = "CMAKE_C_STANDARD_REQUIRED";
	}
#if defined(CHALET_MACOS)
	else if (inProps.language == CodeLanguage::ObjectiveCPlusPlus)
	{
		standard = fmt::format("CMAKE_OBJCXX_STANDARD {}", inProps.langStandard);
		standardRequired = "CMAKE_OBJCXX_STANDARD_REQUIRED";
		extraSettings = R"cmake(
enable_language(OBJCXX))cmake";
		extraProperties = R"cmake(
target_link_libraries(${TARGET_NAME} PRIVATE "-framework Foundation")
)cmake";
	}
	else if (inProps.language == CodeLanguage::ObjectiveC)
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
	else // CPlusPlus
	{
		if (String::equals("26", inProps.langStandard))
			minimumCMakeVersion = "3.25";
		else if (String::equals("23", inProps.langStandard))
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

if (DEFINED EMSCRIPTEN)
	set(CMAKE_EXECUTABLE_SUFFIX ".html")
endif()

set(TARGET_NAME {projectName})
{sources}

add_executable(${{TARGET_NAME}} ${{SOURCES}})
{precompiledHeader}
target_include_directories(${{TARGET_NAME}} PRIVATE {location}/){extraProperties}

if(MSVC)
	target_compile_options(${{TARGET_NAME}} PRIVATE /utf-8)
endif())cmake",
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

/*****************************************************************************/
Json StarterFileTemplates::getMesonStarterChaletJson(const ChaletJsonProps& inProps)
{
	const auto& project = inProps.projectName;

	Json ret;
	ret[Keys::WorkspaceName] = inProps.workspaceName;
	ret[Keys::WorkspaceVersion] = inProps.version;

	ret[Keys::DefaultConfigurations] = Json::array();
	ret[Keys::DefaultConfigurations] = {
		"Release",
		"Debug",
		"MinSizeRel",
		"RelWithDebInfo",
	};

	ret[Keys::Targets] = Json::object();
	ret[Keys::Targets][project] = Json::object();
	ret[Keys::Targets][project][Keys::Kind] = "mesonProject";
	ret[Keys::Targets][project]["location"] = ".";
	ret[Keys::Targets][project]["recheck"] = true;
	ret[Keys::Targets][project]["runExecutable"] = project;

	ret[Keys::Distribution] = Json::object();
	ret[Keys::Distribution][project] = Json::object();
	ret[Keys::Distribution][project][Keys::Kind] = "bundle";
	ret[Keys::Distribution][project]["include"] = Json::array();
	ret[Keys::Distribution][project]["include"][0] = fmt::format("${{externalBuild:{}}}/${{exe:{}}}", project, project);

	return ret;
}

/*****************************************************************************/
std::string StarterFileTemplates::getMesonStarter(const ChaletJsonProps& inProps)
{
	UNUSED(inProps);
	const auto& version = inProps.version;
	const auto& workspaceName = inProps.workspaceName;
	const auto& projectName = inProps.projectName;
	const auto& location = inProps.location;
	auto main = fmt::format("{}/{}", location, inProps.mainSource);
	auto pch = fmt::format("{}/{}", location, inProps.precompiledHeader);
	// auto sourceExt = String::getPathSuffix(inProps.mainSource);

	const bool hasPch = !inProps.precompiledHeader.empty();
	std::string precompiledHeader;
	std::string pchBool = hasPch ? "true" : "false";

	std::string language;
	std::string languageStandard;
	if (inProps.language == CodeLanguage::C)
	{
		language = "c";
		languageStandard = fmt::format("c_std=c{}", inProps.langStandard);
		if (hasPch)
			precompiledHeader = fmt::format(", c_pch: '{}'", pch);
	}
#if defined(CHALET_MACOS)
	else if (inProps.language == CodeLanguage::ObjectiveCPlusPlus)
	{
		language = "objcpp";
		languageStandard = fmt::format("cpp_std=c++{}", inProps.langStandard);
		if (hasPch)
			precompiledHeader = fmt::format(", cpp_pch: '{}'", pch);
	}
	else if (inProps.language == CodeLanguage::ObjectiveC)
	{
		language = "objc";
		languageStandard = fmt::format("c_std=c{}", inProps.langStandard);
		if (hasPch)
			precompiledHeader = fmt::format(", c_pch: '{}'", pch);
	}
#endif
	else // CPlusPlus
	{
		language = "cpp";
		languageStandard = fmt::format("cpp_std=c++{}", inProps.langStandard);
		if (hasPch)
			precompiledHeader = fmt::format(", cpp_pch: '{}'", pch);
	}

	// TODO: Meson needs the .cpp for the PCH on MSVC

	std::string ret = fmt::format(R"python(project('{workspaceName}', '{language}',
	version: '{version}',
	license: 'NONE',
	meson_version: '>=0.5.0',
	default_options: ['b_pch={pchBool}', '{languageStandard}'])

sources = [
	'{main}'
]
executable('{projectName}', sources{precompiledHeader}, install: true))python",
		FMT_ARG(workspaceName),
		FMT_ARG(version),
		FMT_ARG(language),
		FMT_ARG(languageStandard),
		FMT_ARG(projectName),
		FMT_ARG(main),
		FMT_ARG(pchBool),
		FMT_ARG(precompiledHeader),
		FMT_ARG(location));

	return ret;
}
}
