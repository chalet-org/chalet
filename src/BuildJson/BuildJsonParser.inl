/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "BuildJson/BuildJsonParser.hpp"

#include "Libraries/Format.hpp"

namespace chalet
{
/*****************************************************************************/
template <typename T>
bool BuildJsonParser::parseKeyFromConfig(T& outVariable, const Json& inNode, const std::string& inKey)
{
	using Type = std::decay_t<T>;
	static_assert(!std::is_same_v<Type, std::string>, "use assignStringFromConfig instead");

	bool res = m_buildJson->assignFromKey(outVariable, inNode, inKey);

	const auto& platform = m_state.info.platform();

	res |= m_buildJson->assignFromKey(outVariable, inNode, fmt::format("{}.{}", inKey, platform));

	if (m_state.configuration.debugSymbols())
	{
		res |= m_buildJson->assignFromKey(outVariable, inNode, fmt::format("{}:{}", inKey, m_debugIdentifier));
		res |= m_buildJson->assignFromKey(outVariable, inNode, fmt::format("{}:{}.{}", inKey, m_debugIdentifier, platform));
	}
	else
	{
		res |= m_buildJson->assignFromKey(outVariable, inNode, fmt::format("{}:!{}", inKey, m_debugIdentifier));
		res |= m_buildJson->assignFromKey(outVariable, inNode, fmt::format("{}:!{}.{}", inKey, m_debugIdentifier, platform));
	}

	for (auto& notPlatform : m_state.info.notPlatforms())
	{
		res |= m_buildJson->assignFromKey(outVariable, inNode, fmt::format("{}.!{}", inKey, notPlatform));

		if (m_state.configuration.debugSymbols())
			res |= m_buildJson->assignFromKey(outVariable, inNode, fmt::format("{}:{}.!{}", inKey, m_debugIdentifier, notPlatform));
		else
			res |= m_buildJson->assignFromKey(outVariable, inNode, fmt::format("{}:!{}.!{}", inKey, m_debugIdentifier, notPlatform));
	}

	return res;
}
}