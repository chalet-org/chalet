/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_FILE_TEMPLATES_HPP
#define CHALET_FILE_TEMPLATES_HPP

#include "Init/BuildJsonProps.hpp"
#include "Libraries/Json.hpp"

namespace chalet
{
struct FileTemplates
{
	FileTemplates() = delete;

	static Json getBuildJson(const BuildJsonProps& inProps);
	static std::string getMainCpp();
	static std::string getPch();
	static std::string getGitIgnore(const std::string& inBuildFolder);
	static std::string getDotEnv();
};
}

#endif // CHALET_FILE_TEMPLATES_HPP
