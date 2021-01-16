/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Json/JsonParser.hpp"

#include "Terminal/Output.hpp"
#include "Utility/List.hpp"
#include "Json/JsonNode.hpp"
#include "Json/JsonValidator.hpp"

namespace chalet
{
/*****************************************************************************/
bool JsonParser::assignStringAndValidate(std::string& outString, const Json& inNode, const std::string& inKey, const std::string& inDefault)
{
	bool result = JsonNode::assignFromKey(outString, inNode, inKey);
	if (result && outString.empty())
	{
		Output::warnBlankKey(inKey, inDefault);
		return false;
	}

	return result;
}

/*****************************************************************************/
bool JsonParser::assignStringListAndValidate(StringList& outList, const Json& inNode, const std::string& inKey)
{
	if (!inNode.contains(inKey))
		return false;

	const Json& list = inNode.at(inKey);
	if (!list.is_array())
		return false;

	for (auto& itemRaw : list)
	{
		if (!itemRaw.is_string())
			return false;

		std::string item = itemRaw.get<std::string>();
		if (item.empty())
			Output::warnBlankKeyInList(inKey);

		List::addIfDoesNotExist(outList, std::move(item));
	}

	return true;
}

}
