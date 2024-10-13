/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Json/JsonFile.hpp"

namespace chalet
{
/*****************************************************************************/
template <typename T>
inline bool JsonFile::assignNodeIfEmpty(Json& outNode, const char* inKey, const T& inValue)
{
	bool result = false;
	bool notFound = !outNode.contains(inKey);

	using Type = std::decay_t<T>;
	if constexpr (std::is_same_v<Type, std::string>)
	{
		if (notFound || !outNode[inKey].is_string() || outNode[inKey].get<std::string>().empty())
		{
			outNode[inKey] = inValue;
			result = true;
		}
	}
	else if constexpr (std::is_same_v<Type, bool>)
	{
		if (notFound || !outNode[inKey].is_boolean())
		{
			outNode[inKey] = inValue;
			result = true;
		}
	}
	else if constexpr (std::is_unsigned_v<Type>)
	{
		if (notFound || !outNode[inKey].is_number_unsigned())
		{
			outNode[inKey] = inValue;
			result = true;
		}
	}
	else if constexpr (std::is_floating_point_v<Type>)
	{
		if (notFound || !outNode[inKey].is_number_float())
		{
			outNode[inKey] = inValue;
			result = true;
		}
	}
	else if constexpr (std::is_integral_v<Type>)
	{
		if (notFound || !outNode[inKey].is_number_integer())
		{
			outNode[inKey] = inValue;
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
inline bool JsonFile::assignNodeIfEmptyWithFallback(Json& outNode, const char* inKey, const std::optional<T>& inValueA, const T& inValueB)
{
	bool result = false;
	bool valid = json::isValid<T>(outNode, inKey) && !inValueA.has_value();

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
}