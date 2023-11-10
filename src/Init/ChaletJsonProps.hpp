/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include "Compile/CodeLanguage.hpp"

namespace chalet
{
struct ChaletJsonProps
{
	std::string workspaceName;
	std::string version;
	std::string projectName;
	std::string location;
	std::string mainSource;
	std::string precompiledHeader;
	std::string langStandard;
	CodeLanguage language = CodeLanguage::None;
	bool modules = false;
	bool makeGitRepository = false;
	bool useLocation = true;
	bool defaultConfigs = true;
	bool envFile = true;
};
}
