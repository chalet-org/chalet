/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_JSON_PARSER_HPP
#define CHALET_JSON_PARSER_HPP

#include "Libraries/Json.hpp"
#include "State/CommandLineInputs.hpp"
#include "Json/JsonNode.hpp"

namespace chalet
{
struct JsonParser
{
	JsonParser() = default;
	virtual ~JsonParser() = default;

	virtual bool serialize() = 0;

protected:
	bool assignStringAndValidate(std::string& outString, const Json& inNode, const std::string& inKey, const std::string& inDefault = "");
	bool assignStringListAndValidate(StringList& outList, const Json& inNode, const std::string& inKey);
};
}

#endif // CHALET_JSON_PARSER_HPP
