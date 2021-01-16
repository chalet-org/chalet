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
Json parse(const std::string& inFilename);
Json parseLiteral(const std::string& inJsonContent);
}
}

#endif // CHALET_JSON_COMMENTS_HPP
