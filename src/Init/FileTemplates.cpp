/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Init/FileTemplates.hpp"

#include "Libraries/Format.hpp"
#include "Utility/String.hpp"
#include "Json/JsonComments.hpp"

namespace chalet
{
/*****************************************************************************/
Json FileTemplates::getBuildJson(const BuildJsonProps& inProps)
{
	const std::string language = inProps.language == CodeLanguage::CPlusPlus ?
		"C++" :
		"C";

	const std::string langStandardKey = inProps.language == CodeLanguage::CPlusPlus ?
		"cppStandard" :
		"cStandard";

	const std::string langStandardValue = inProps.language == CodeLanguage::CPlusPlus ?
		"c++17" :
		"c17";

	const std::string kind = "consoleApplication";

	std::string ret = R"json({
	"version": "${version}",
	"workspace": "${workspace}",
	"allProjects": {
		"language": "${language}",
		"${langStandardKey}": "${langStandardValue}",
		"warnings": "pedantic"
	},
	"projects": [
		{
			"name": "${project}",
			"kind": "${kind}",
			"location": "src",
			"pch": "src/PCH.hpp",
			"runProject": true
		}
	]
}
)json";

	String::replaceAll(ret, "${version}", inProps.version);
	String::replaceAll(ret, "${workspace}", inProps.workspaceName);
	String::replaceAll(ret, "${project}", inProps.projectName);
	String::replaceAll(ret, "${language}", language);
	String::replaceAll(ret, "${langStandardKey}", langStandardKey);
	String::replaceAll(ret, "${langStandardValue}", langStandardValue);
	String::replaceAll(ret, "${kind}", kind);

	return JsonComments::parseLiteral(ret);
}

/*****************************************************************************/
std::string FileTemplates::getMainCpp()
{
	std::string ret = R"(#include <iostream>

int main(const int argc, const char* const argv[])
{
	std::cout << "Hello world!\n\n";
	std::cout << "Args:\n";

	for (int i=0; i < argc; ++i)
	{
		std::cout << "  " << argv[i] << "\n";
	}

	std::cout << std::flush;

	return 0;
}

)";

	return ret;
}

/*****************************************************************************/
std::string FileTemplates::getPch()
{

	std::string ret = R"(#ifndef PRECOMPILED_HEADER_HPP
#define PRECOMPILED_HEADER_HPP

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

#endif // PRECOMPILED_HEADER_HPP
)";

	return ret;
}

/*****************************************************************************/
std::string FileTemplates::getGitIgnore()
{
	std::string ret = R"(# General
Thumbs.db
.DS_Store

# Build
bin
build
chalet_external/
)";

	return ret;
}

}
