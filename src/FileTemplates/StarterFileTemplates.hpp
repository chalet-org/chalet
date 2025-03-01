/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include "Init/ChaletJsonProps.hpp"
#include "Libraries/Json.hpp"

namespace chalet
{
namespace StarterFileTemplates
{
Json getStandardChaletJson(const ChaletJsonProps& inProps);
std::string getMainCxx(const CodeLanguage inLanguage, const std::string& inStandard, const bool inModules);
std::string getPch(const std::string& inFile, const CodeLanguage inLanguage);
std::string getGitIgnore(const std::string& inBuildFolder, const std::string& inSettingsFile);
std::string getDotEnv();
//
Json getCMakeStarterChaletJson(const ChaletJsonProps& inProps);
std::string getCMakeStarter(const ChaletJsonProps& inProps);
//
Json getMesonStarterChaletJson(const ChaletJsonProps& inProps);
std::string getMesonStarter(const ChaletJsonProps& inProps);
}
}
