/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_BUILD_JSON_PROPS_HPP
#define CHALET_BUILD_JSON_PROPS_HPP

#include "BuildJson/CompileEnvironment.hpp"

namespace chalet
{
struct BuildJsonProps
{
	std::string workspaceName;
	std::string version;
	std::string projectName;
	CodeLanguage language = CodeLanguage::CPlusPlus;
};
}

#endif // CHALET_BUILD_JSON_PROPS_HPP
