/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Check/BuildFileChecker.hpp"

namespace chalet
{
/*****************************************************************************/
template <typename T>
bool BuildFileChecker::checkNodeWithTargetPtr(Json& node, const T* inTarget)
{
	if (node.is_object())
	{
		for (const auto& [keyRaw, value] : node.items())
		{
			checkNodeWithTargetPtr<T>(value, inTarget);
		}
	}
	else if (node.is_array())
	{
		for (auto& value : node)
		{
			checkNodeWithTargetPtr<T>(value, inTarget);
		}
	}
	else if (node.is_string())
	{
		auto value = node.get<std::string>();
		m_state.replaceVariablesInString(value, inTarget);
		node = value;
	}

	return true;
}
}
