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

/*****************************************************************************/
template <typename T>
bool BuildJsonParser::parseKeyWithToolchain(T& outVariable, const Json& inNode, const std::string& inKey) const
{
	chalet_assert(m_state.environment != nullptr, "");

	if (parseKeyFromConfig(outVariable, inNode, inKey))
	{
		return true;
	}
	else if (inNode.contains(inKey))
	{
		auto& innerNode = inNode.at(inKey);
		if (innerNode.is_object())
		{
			const auto& triple = m_state.info.targetArchitectureTriple();
			const auto& toolchainName = m_inputs.toolchainPreferenceName();

			bool res = parseKeyFromConfig(outVariable, innerNode, "*");

			if (triple != toolchainName)
				res |= parseKeyFromConfig(outVariable, innerNode, triple);

			res |= parseKeyFromConfig(outVariable, innerNode, toolchainName);

			return res;
		}
	}

	return false;
}
}