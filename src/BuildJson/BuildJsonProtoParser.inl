/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "BuildJson/BuildJsonProtoParser.hpp"

namespace chalet
{
/*****************************************************************************/
template <typename T>
bool BuildJsonProtoParser::valueMatchesSearchKeyPattern(T& outVariable, const Json& inNode, const std::string& inKey, const char* inSearch, JsonNodeReadStatus& inStatus) const
{
	if (!String::equals(inSearch, inKey))
	{
		if (!String::startsWith(fmt::format("{}.", inSearch), inKey))
			return false;

		inStatus = JsonNodeReadStatus::ValidKeyUnreadValue;

		for (auto& notPlatform : m_notPlatforms)
		{
			if (String::contains(fmt::format(".{}", notPlatform), inKey))
				return false;
		}

		const auto ci = Environment::isContinuousIntegrationServer() ? "" : "!";
		if (!String::contains(fmt::format(".{}ci", ci), inKey))
			return false;
	}

	// LOG(inKey);
	inStatus = JsonNodeReadStatus::ValidKeyReadValue;
	using Type = std::decay_t<T>;
	if constexpr (std::is_same<Type, StringList>())
	{
		for (auto& itemRaw : inNode)
		{
			if (!itemRaw.is_string())
				return false;

			std::string item = itemRaw.template get<std::string>();
			List::addIfDoesNotExist(outVariable, std::move(item));
		}
	}
	else
	{
		outVariable = inNode.template get<Type>();
	}
	return true;
}
}
