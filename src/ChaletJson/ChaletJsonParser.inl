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
	if (!String::equals(inSearch, inKey))
	{
		if (!String::startsWith(fmt::format("{}[", inSearch), inKey))
			return false;

		inStatus = JsonNodeReadStatus::ValidKeyUnreadValue;

		if (!m_adapter.matchConditionVariables(inKey, [this, &inKey, &inStatus](const std::string& key, const std::string& value, bool negate) {
				auto res = checkConditionVariable(inKey, key, value, negate);
				if (res == ConditionResult::Invalid)
					inStatus = JsonNodeReadStatus::Invalid;

				return res == ConditionResult::Pass;
			}))
		{
			if (m_adapter.lastOp == ConditionOp::InvalidOr)
			{
				inStatus = JsonNodeReadStatus::Invalid;
				Diagnostic::error("Syntax for AND '+', OR '|' are mutually exclusive. Both found in: {}", inKey);
			}
			return false;
		}
	}

	/*if (String::startsWith(fmt::format("{}.", inSearch), inKey))
	{
		LOG(inKey);
	}*/

	inStatus = JsonNodeReadStatus::ValidKeyReadValue;
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