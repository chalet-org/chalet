/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "ChaletJson/ChaletJsonParser.hpp"

#include "Core/CommandLineInputs.hpp"

namespace chalet
{
/*****************************************************************************/
template <typename T>
bool ChaletJsonParser::valueMatchesSearchKeyPattern(T& outVariable, const Json& inNode, const std::string& inKey, const char* inSearch, JsonNodeReadStatus& inStatus) const
{
	if (!valueMatchesSearchKeyPattern(inKey, inSearch, inStatus))
		return false;

	using Type = std::decay_t<T>;
	if constexpr (std::is_same<Type, StringList>())
	{
		for (auto& item : inNode)
		{
			if (item.is_string())
			{
				outVariable.emplace_back(item.template get<std::string>());
			}
		}
	}
	else
	{
		outVariable = inNode.template get<Type>();
	}
	return true;
}
}