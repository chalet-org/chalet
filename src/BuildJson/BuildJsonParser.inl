/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "BuildJson/BuildJsonParser.hpp"

#include "Libraries/Format.hpp"
#include "Json/JsonNode.hpp"

namespace chalet
{
/*****************************************************************************/
template <typename T>
bool BuildJsonParser::parseKeyFromConfig(T& outVariable, const Json& inNode, const std::string& inKey)
{
	using Type = std::decay_t<T>;
	static_assert(!std::is_same_v<Type, std::string>, "use assignStringFromConfig instead");

	bool res = JsonNode::assignFromKey(outVariable, inNode, inKey);

	const auto& platform = m_state.platform();

	res |= JsonNode::assignFromKey(outVariable, inNode, fmt::format("{}.{}", inKey, platform));

	if (m_state.configuration.debugSymbols())
	{
		res |= JsonNode::assignFromKey(outVariable, inNode, fmt::format("{}:{}", inKey, m_debugIdentifier));
		res |= JsonNode::assignFromKey(outVariable, inNode, fmt::format("{}:{}.{}", inKey, m_debugIdentifier, platform));
	}
	else
	{
		res |= JsonNode::assignFromKey(outVariable, inNode, fmt::format("{}:!{}", inKey, m_debugIdentifier));
		res |= JsonNode::assignFromKey(outVariable, inNode, fmt::format("{}:!{}.{}", inKey, m_debugIdentifier, platform));
	}

	for (auto& notPlatform : m_state.notPlatforms())
	{
		res |= JsonNode::assignFromKey(outVariable, inNode, fmt::format("{}.!{}", inKey, notPlatform));

		if (m_state.configuration.debugSymbols())
			res |= JsonNode::assignFromKey(outVariable, inNode, fmt::format("{}:{}.!{}", inKey, m_debugIdentifier, notPlatform));
		else
			res |= JsonNode::assignFromKey(outVariable, inNode, fmt::format("{}:!{}.!{}", inKey, m_debugIdentifier, notPlatform));
	}

	return res;
}
}