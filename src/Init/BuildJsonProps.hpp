/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_BUILD_JSON_PROPS_HPP
#define CHALET_BUILD_JSON_PROPS_HPP

#include "Compile/CodeLanguage.hpp"
#include "Compile/CxxSpecialization.hpp"

namespace chalet
{
struct BuildJsonProps
{
	std::string workspaceName;
	std::string version;
	std::string projectName;
	std::string location;
	std::string mainSource;
	std::string precompiledHeader;
	std::string langStandard;
	CodeLanguage language = CodeLanguage::None;
	CxxSpecialization specialization = CxxSpecialization::CPlusPlus;
	bool makeGitRepository = false;
	bool useLocation = true;
	bool defaultConfigs = true;
	bool envFile = true;
};
}

#endif // CHALET_BUILD_JSON_PROPS_HPP
