/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Json/JsonNode.hpp"

#include "Libraries/Format.hpp"
#include "State/CommandLineInputs.hpp"

namespace chalet
{
/*****************************************************************************/
template <typename T>
bool JsonNode::assignFromKey(T& outVariable, const Json& inNode, const std::string& inKey)
{
	if (!inNode.contains(inKey))
		return false;

	if (inNode.at(inKey).is_null())
	{
		Diagnostic::errorAbort(fmt::format("{}: An invalid value (null) was found in '{}'.", CommandLineInputs::file(), inKey));
		return false;
	}
	else if (inNode.at(inKey).is_object())
	{
		Diagnostic::errorAbort(fmt::format("{}: An invalid value (object) was found in '{}'.", CommandLineInputs::file(), inKey));
		return false;
	}
	else if (inNode.at(inKey).is_array())
	{
		Diagnostic::errorAbort(fmt::format("{}: An invalid value (array) was found in '{}'.", CommandLineInputs::file(), inKey));
		return false;
	}

	using Type = std::decay_t<T>;
	if constexpr (std::is_same_v<Type, std::string>)
	{
		if (!inNode.at(inKey).is_string())
		{
			Diagnostic::errorAbort(fmt::format("{}: An invalid value was found in '{}'. Expected string", CommandLineInputs::file(), inKey));
			return false;
		}
	}
	else if constexpr (std::is_same_v<Type, bool>)
	{
		if (!inNode.at(inKey).is_boolean())
		{
			Diagnostic::errorAbort(fmt::format("{}: An invalid value was found in '{}'. Expected true|false", CommandLineInputs::file(), inKey));
			return false;
		}
	}
	else if constexpr (std::is_unsigned_v<Type>)
	{
		if (!inNode.at(inKey).is_number_unsigned())
		{
			Diagnostic::errorAbort(fmt::format("{}: An invalid value was found in '{}'. Expected unsigned integer", CommandLineInputs::file(), inKey));
			return false;
		}
	}
	else if constexpr (std::is_floating_point_v<Type>)
	{
		if (!inNode.at(inKey).is_number_float())
		{
			Diagnostic::errorAbort(fmt::format("{}: An invalid value was found in '{}'. Expected floating point", CommandLineInputs::file(), inKey));
			return false;
		}
	}
	else if constexpr (std::is_integral_v<Type>)
	{
		if (!inNode.at(inKey).is_number_integer())
		{
			Diagnostic::errorAbort(fmt::format("{}: An invalid value was found in '{}'. Expected integer", CommandLineInputs::file(), inKey));
			return false;
		}
	}

	outVariable = inNode.at(inKey).template get<Type>();
	return true;
}
}