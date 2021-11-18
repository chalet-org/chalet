/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_STARTER_FILE_TEMPLATES_HPP
#define CHALET_STARTER_FILE_TEMPLATES_HPP

#include "Compile/CxxSpecialization.hpp"
#include "Init/BuildJsonProps.hpp"
#include "Libraries/Json.hpp"

namespace chalet
{
namespace StarterFileTemplates
{
Json getStandardChaletJson(const BuildJsonProps& inProps);
std::string getMainCxx(const CxxSpecialization inSpecialization, const bool inModules);
std::string getPch(const std::string& inFile, const CodeLanguage inLanguage, const CxxSpecialization inSpecialization);
std::string getGitIgnore(const std::string& inBuildFolder, const std::string& inSettingsFile);
std::string getDotEnv();
//
Json getCMakeStarterChaletJson(const BuildJsonProps& inProps);
std::string getCMakeStarter(const BuildJsonProps& inProps);
}
}

#endif // CHALET_STARTER_FILE_TEMPLATES_HPP
