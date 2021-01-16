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
	static_assert(!std::is_same_v<T, std::string>, "use assignStringFromConfig instead");
	bool res = JsonNode::assignFromKey(outVariable, inNode, inKey);

	const auto& buildConfiguration = m_state.buildConfiguration();
	const auto& platform = m_state.platform();

	res |= JsonNode::assignFromKey(outVariable, inNode, fmt::format("{}:{}", inKey, buildConfiguration));
	res |= JsonNode::assignFromKey(outVariable, inNode, fmt::format("{}.{}", inKey, platform));
	res |= JsonNode::assignFromKey(outVariable, inNode, fmt::format("{}:{}.{}", inKey, buildConfiguration, platform));

	return res;
}
}