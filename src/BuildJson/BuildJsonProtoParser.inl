/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "BuildJson/BuildJsonProtoParser.hpp"

namespace chalet
{
/*****************************************************************************/
template <typename T>
bool BuildJsonProtoParser::parseKeyFromConfig(T& outVariable, const Json& inNode, const std::string& inKey) const
{
	bool res = m_chaletJson.assignFromKey(outVariable, inNode, inKey);

	const auto& platform = m_platform;

	res |= m_chaletJson.assignFromKey(outVariable, inNode, fmt::format("{}.{}", inKey, platform));

	for (auto& notPlatform : m_notPlatforms)
	{
		res |= m_chaletJson.assignFromKey(outVariable, inNode, fmt::format("{}.!{}", inKey, notPlatform));
	}

	return res;
}
}
