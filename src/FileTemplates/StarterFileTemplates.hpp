/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_STARTER_FILE_TEMPLATES_HPP
#define CHALET_STARTER_FILE_TEMPLATES_HPP

#include "Init/BuildJsonProps.hpp"
#include "Libraries/Json.hpp"

namespace chalet
{
namespace StarterFileTemplates
{
Json getBuildJson(const BuildJsonProps& inProps);
std::string getMainCpp();
std::string getPch();
std::string getGitIgnore(const std::string& inBuildFolder);
std::string getDotEnv();
}
}

#endif // CHALET_STARTER_FILE_TEMPLATES_HPP
