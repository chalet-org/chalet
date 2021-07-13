/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Json/JsonFile.hpp"

#include "Core/CommandLineInputs.hpp"

namespace chalet
{
/*****************************************************************************/
template <typename T>
bool JsonFile::assignFromKey(T& outVariable, const Json& inNode, const std::string& inKey)
{
	if (!containsKeyForType<T>(inNode, inKey))
		return false;

	using Type = std::decay_t<T>;
	outVariable = inNode.at(inKey).template get<Type>();
	return true;
}

/*****************************************************************************/
template <typename T>
bool JsonFile::assignNodeIfEmpty(Json& outNode, const std::string& inKey, const std::function<T()>& onAssign)
{
	bool result = false;
	bool notFound = !outNode.contains(inKey);

	using Type = std::decay_t<T>;
	if constexpr (std::is_same_v<Type, std::string>)
	{
		if (notFound || !outNode.at(inKey).is_string())
		{
			outNode[inKey] = onAssign();
			result = true;
		}
	}
	else if constexpr (std::is_same_v<Type, bool>)
	{
		if (notFound || !outNode.at(inKey).is_boolean())
		{
			outNode[inKey] = onAssign();
			result = true;
		}
	}
	else if constexpr (std::is_unsigned_v<Type>)
	{
		if (notFound || !outNode.at(inKey).is_number_unsigned())
		{
			outNode[inKey] = onAssign();
			result = true;
		}
	}
	else if constexpr (std::is_floating_point_v<Type>)
	{
		if (notFound || !outNode.at(inKey).is_number_float())
		{
			outNode[inKey] = onAssign();
			result = true;
		}
	}
	else if constexpr (std::is_integral_v<Type>)
	{
		if (notFound || !outNode.at(inKey).is_number_integer())
		{
			outNode[inKey] = onAssign();
			result = true;
		}
	}
	else
	{
		chalet_assert(false, "JsonFile::assignNodeIfEmpty - invalid type");
	}

	if (result)
	{
		setDirty(true);
	}

	return result;
}

/*****************************************************************************/
template <typename T>
bool JsonFile::containsKeyForType(const Json& inNode, const std::string& inKey)
{
	if (!inNode.contains(inKey))
		return false;

	if (inNode.at(inKey).is_null())
	{
		Diagnostic::error("{}: An invalid value (null) was found in '{}'.", m_filename, inKey);
		return false;
	}
	else if (inNode.at(inKey).is_object())
	{
		Diagnostic::error("{}: An invalid value (object) was found in '{}'.", m_filename, inKey);
		return false;
	}
	else if (inNode.at(inKey).is_array())
	{
		Diagnostic::error("{}: An invalid value (array) was found in '{}'.", m_filename, inKey);
		return false;
	}

	using Type = std::decay_t<T>;
	if constexpr (std::is_same_v<Type, std::string>)
	{
		if (!inNode.at(inKey).is_string())
		{
			Diagnostic::error("{}: An invalid value was found in '{}'. Expected string", m_filename, inKey);
			return false;
		}
	}
	else if constexpr (std::is_same_v<Type, bool>)
	{
		if (!inNode.at(inKey).is_boolean())
		{
			Diagnostic::error("{}: An invalid value was found in '{}'. Expected true|false", m_filename, inKey);
			return false;
		}
	}
	else if constexpr (std::is_unsigned_v<Type>)
	{
		if (!inNode.at(inKey).is_number_unsigned())
		{
			Diagnostic::error("{}: An invalid value was found in '{}'. Expected unsigned integer", m_filename, inKey);
			return false;
		}
	}
	else if constexpr (std::is_floating_point_v<Type>)
	{
		if (!inNode.at(inKey).is_number_float())
		{
			Diagnostic::error("{}: An invalid value was found in '{}'. Expected floating point", m_filename, inKey);
			return false;
		}
	}
	else if constexpr (std::is_integral_v<Type>)
	{
		if (!inNode.at(inKey).is_number_integer())
		{
			Diagnostic::error("{}: An invalid value was found in '{}'. Expected integer", m_filename, inKey);
			return false;
		}
	}
	else
	{
		chalet_assert(false, "JsonFile::containsKeyForType - invalid type");
	}

	return true;
}
}