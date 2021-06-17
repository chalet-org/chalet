/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "State/StatePrototype.hpp"

namespace chalet
{
/*****************************************************************************/
template <typename T>
bool StatePrototype::parseKeyFromConfig(T& outVariable, const Json& inNode, const std::string& inKey)
{
	using Type = std::decay_t<T>;
	static_assert(!std::is_same_v<Type, std::string>, "use assignStringFromConfig instead");

	bool res = m_buildJson->assignFromKey(outVariable, inNode, inKey);

	const auto& platform = m_inputs.platform();

	res |= m_buildJson->assignFromKey(outVariable, inNode, fmt::format("{}.{}", inKey, platform));

	for (auto& notPlatform : m_inputs.notPlatforms())
	{
		res |= m_buildJson->assignFromKey(outVariable, inNode, fmt::format("{}.!{}", inKey, notPlatform));
	}

	return res;
}
}