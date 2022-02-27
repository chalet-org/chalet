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
		if (!String::startsWith(fmt::format("{}.", inSearch), inKey))
			return false;

		inStatus = JsonNodeReadStatus::ValidKeyUnreadValue;

		if (String::contains(fmt::format(".!{}", m_platform), inKey))
			return false;

		for (auto& notPlatform : m_notPlatforms)
		{
			if (String::contains(fmt::format(".{}", notPlatform), inKey))
				return false;
		}

		/*if (String::contains(fmt::format(".!{}", m_toolchain), inKey))
			return false;

		for (auto& notPlatform : m_notToolchains)
		{
			if (String::contains(fmt::format(".{}", notPlatform), inKey))
				return false;
		}*/

		const auto debug = m_state.configuration.debugSymbols() ? "!" : "";
		if (String::contains(fmt::format(".{}debug", debug), inKey))
			return false;

		// const auto ci = Environment::isContinuousIntegrationServer() ? "!" : "";
		// if (String::contains(fmt::format(".{}ci", ci), inKey))
		// 	return true;
	}

	// LOG(inKey);
	inStatus = JsonNodeReadStatus::ValidKeyReadValue;
	using Type = std::decay_t<T>;
	if constexpr (std::is_same<Type, StringList>())
	{
		for (auto& item : inNode)
		{
			if (item.is_string())
			{
				List::addIfDoesNotExist(outVariable, item.template get<std::string>());
			}
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
bool ChaletJsonParser::valueMatchesToolchainSearchPattern(T& outVariable, const Json& inNode, const std::string& inKey, const char* inSearch, JsonNodeReadStatus& inStatus) const
{
	chalet_assert(m_state.environment != nullptr, "");

	if (!String::equals(inSearch, inKey))
		return false;

	using Type = std::decay_t<T>;

	const auto& triple = m_state.info.targetArchitectureTriple();
	const auto& toolchainName = m_state.inputs.toolchainPreferenceName();

	inStatus = JsonNodeReadStatus::ValidKeyUnreadValue;

	bool res = true;
	if constexpr (std::is_same<Type, StringList>())
	{
		for (const auto& [key, value] : inNode.items())
		{
			if (value.is_array())
			{
				if (String::equals("*", key) || String::contains(key, triple) || String::contains(key, toolchainName))
				{
					for (auto& item : value)
					{
						if (item.is_string())
						{
							List::addIfDoesNotExist(outVariable, item.template get<std::string>());
						}
					}
				}
			}
		}
	}
	else if constexpr (std::is_same<Type, std::string>())
	{
		std::string all;
		for (const auto& [key, value] : inNode.items())
		{
			if (value.is_string())
			{
				if (String::equals("*", key))
				{
					all = value.template get<Type>();
				}
				else if (String::contains(key, triple) || String::contains(key, toolchainName))
				{
					outVariable = value.template get<Type>();
				}
			}
		}
		if (!all.empty() && outVariable.empty())
		{
			outVariable = std::move(all);
		}
	}
	else
	{
		res = false;
	}

	if constexpr (std::is_same<Type, StringList>() || std::is_same<Type, std::string>())
	{
		res = !outVariable.empty();
		if (res)
			inStatus = JsonNodeReadStatus::ValidKeyReadValue;
	}

	return res;
}
}