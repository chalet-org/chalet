/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

// Note: mind the order
#include "Core/CommandLineInputs.hpp"
#include "State/BuildState.hpp"
#include "State/Dependency/GitDependency.hpp"
//
#include "BuildJson/BuildJsonProtoParser.hpp"

namespace chalet
{
/*****************************************************************************/
template <typename T>
bool BuildJsonProtoParser::parseKeyFromConfig(T& outVariable, const Json& inNode, const std::string& inKey) const
{
	bool res = m_buildJson.assignFromKey(outVariable, inNode, inKey);

	const auto& platform = m_inputs.platform();

	res |= m_buildJson.assignFromKey(outVariable, inNode, fmt::format("{}.{}", inKey, platform));

	for (auto& notPlatform : m_inputs.notPlatforms())
	{
		res |= m_buildJson.assignFromKey(outVariable, inNode, fmt::format("{}.!{}", inKey, notPlatform));
	}

	return res;
}
}
