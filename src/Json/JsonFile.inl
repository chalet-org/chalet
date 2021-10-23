/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Json/JsonFile.hpp"

namespace chalet
{
/*****************************************************************************/
template <typename T>
bool JsonFile::assignFromKey(T& outVariable, const Json& inNode, const std::string& inKey) const
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
		if (notFound || !outNode.at(inKey).is_string() || outNode.at(inKey).get<std::string>().empty())
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

template <typename T>
bool JsonFile::assignNodeIfEmptyWithFallback(Json& outNode, const std::string& inKey, const std::optional<T>& inValueA, const T& inValueB)
{
	bool result = false;
	bool valid = containsKeyForType<T>(outNode, inKey) && !inValueA.has_value();

	if (!valid)
	{
		if (inValueA.has_value())
		{
			outNode[inKey] = *inValueA;
		}
		else
		{
			outNode[inKey] = inValueB;
		}
		setDirty(true);
	}

	return result;
}

/*****************************************************************************/
template <typename T>
bool JsonFile::containsKeyForType(const Json& inNode, const std::string& inKey) const
{
	if (!inNode.contains(inKey))
		return false;

	if (inNode.at(inKey).is_null())
	{
		return false;
	}
	else if (inNode.at(inKey).is_object())
	{
		return false;
	}
	else if (inNode.at(inKey).is_array())
	{
		return false;
	}

	using Type = std::decay_t<T>;
	if constexpr (std::is_same_v<Type, std::string>)
	{
		if (!inNode.at(inKey).is_string())
			return false;
	}
	else if constexpr (std::is_same_v<Type, bool>)
	{
		if (!inNode.at(inKey).is_boolean())
			return false;
	}
	else if constexpr (std::is_unsigned_v<Type>)
	{
		if (!inNode.at(inKey).is_number_unsigned())
			return false;
	}
	else if constexpr (std::is_floating_point_v<Type>)
	{
		if (!inNode.at(inKey).is_number_float())
			return false;
	}
	else if constexpr (std::is_integral_v<Type>)
	{
		if (!inNode.at(inKey).is_number_integer())
			return false;
	}
	else
	{
		chalet_assert(false, "JsonFile::containsKeyForType - invalid type");
		return false;
	}

	return true;
}
}