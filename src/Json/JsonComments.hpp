/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_JSON_COMMENTS_HPP
#define CHALET_JSON_COMMENTS_HPP

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

#endif // CHALET_JSON_COMMENTS_HPP
