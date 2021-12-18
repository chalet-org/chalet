/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "BuildJson/BuildJsonParser.hpp"

namespace chalet
{
/*****************************************************************************/
template <typename T>
bool BuildJsonParser::valueMatchesSearchKeyPattern(T& outVariable, const Json& inNode, const std::string& inKey, const char* inSearch, JsonNodeReadStatus& inStatus) const
{
	if (!String::equals(inSearch, inKey))
	{
		if (!String::startsWith(fmt::format("{}.", inSearch), inKey))
			return false;

		inStatus = JsonNodeReadStatus::ValidKeyUnreadValue;

		for (auto& notPlatform : m_notPlatforms)
		{
			if (String::contains(fmt::format(".{}", notPlatform), inKey))
				return false;
		}

		const auto debug = m_state.configuration.debugSymbols() ? "" : "!";
		if (!String::contains(fmt::format(".{}debug", debug), inKey))
			return false;

		// const auto ci = Environment::isContinuousIntegrationServer() ? "" : "!";
		// if (!String::contains(fmt::format(".{}ci", ci), inKey))
		// 	return false;
	}

	// LOG(inKey);
	inStatus = JsonNodeReadStatus::ValidKeyReadValue;
	using Type = std::decay_t<T>;
	if constexpr (std::is_same<Type, StringList>())
	{
		for (auto& item : inNode)
		{
			if (!item.is_string())
				return false;

			List::addIfDoesNotExist(outVariable, item.template get<std::string>());
		}
	}
	else
	{
		outVariable = inNode.template get<Type>();
	}
	return true;
}

/*****************************************************************************/
template <typename T>
bool BuildJsonParser::valueMatchesToolchainSearchPattern(T& outVariable, const Json& inNode, const std::string& inKey, const char* inSearch, JsonNodeReadStatus& inStatus) const
{
	chalet_assert(m_state.environment != nullptr, "");

	if (String::equals(inSearch, inKey))
	{
		using Type = std::decay_t<T>;

		const auto& triple = m_state.info.targetArchitectureTriple();
		const auto& toolchainName = m_inputs.toolchainPreferenceName();

		inStatus = JsonNodeReadStatus::ValidKeyUnreadValue;

		bool res = true;
		for (const auto& [key, value] : inNode.items())
		{
			if constexpr (std::is_same<Type, StringList>())
			{
				if (value.is_array())
				{
					if (String::equals("*", key) || String::equals(triple, key) || String::equals(toolchainName, key))
					{
						for (auto& item : value)
						{
							if (!item.is_string())
								return false;

							List::addIfDoesNotExist(outVariable, item.template get<std::string>());
						}
					}
				}
			}
			else if constexpr (std::is_same<Type, std::string>())
			{
				if (value.is_string())
				{
					if (String::equals("*", key) || String::equals(triple, key) || String::equals(toolchainName, key))
					{
						outVariable = value.template get<Type>();
					}
				}
			}
			else
			{
				res = false;
			}
		}

		if constexpr (std::is_same<Type, StringList>() || std::is_same<Type, std::string>())
		{
			res = !outVariable.empty();
			inStatus = JsonNodeReadStatus::ValidKeyReadValue;
		}

		return res;
	}

	return false;
}
}