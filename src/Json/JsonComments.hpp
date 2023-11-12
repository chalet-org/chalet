/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include "Libraries/Json.hpp"

namespace chalet
{
namespace JsonComments
{
bool printLinesWithError(std::basic_istream<char>& inContents, const char* inError, std::string& outOutput);
bool parse(Json& outJson, const std::string& inFilename, const bool inError = true);
Json parseLiteral(const std::string& inJsonContent);
}
}
