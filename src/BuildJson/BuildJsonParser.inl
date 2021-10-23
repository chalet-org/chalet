/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "BuildJson/BuildJsonParser.hpp"

namespace chalet
{
/*****************************************************************************/
template <typename T>
bool BuildJsonParser::parseKeyFromConfig(T& outVariable, const Json& inNode, const std::string& inKey) const
{
	bool res = m_chaletJson.assignFromKey(outVariable, inNode, inKey);

	const auto& platform = Platform::platform();

	res |= m_chaletJson.assignFromKey(outVariable, inNode, fmt::format("{}.{}", inKey, platform));

	const auto notSymbol = m_state.configuration.debugSymbols() ? "" : "!";
	res |= m_chaletJson.assignFromKey(outVariable, inNode, fmt::format("{}.{}{}", inKey, notSymbol, m_debugIdentifier));
	res |= m_chaletJson.assignFromKey(outVariable, inNode, fmt::format("{}.{}.{}{}", inKey, platform, notSymbol, m_debugIdentifier));
	res |= m_chaletJson.assignFromKey(outVariable, inNode, fmt::format("{}.{}{}.{}", inKey, notSymbol, m_debugIdentifier, platform));

	for (auto& notPlatform : Platform::notPlatforms())
	{
		res |= m_chaletJson.assignFromKey(outVariable, inNode, fmt::format("{}.!{}", inKey, notPlatform));
		res |= m_chaletJson.assignFromKey(outVariable, inNode, fmt::format("{}.!{}.{}{}", inKey, notPlatform, notSymbol, m_debugIdentifier));
		res |= m_chaletJson.assignFromKey(outVariable, inNode, fmt::format("{}.{}{}.!{}", inKey, notSymbol, m_debugIdentifier, notPlatform));
	}

	return res;
}
}