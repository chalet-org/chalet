#include "Libraries/Json.hpp"

namespace chalet
{
/*****************************************************************************/
template <typename T>
constexpr bool json::isValid(const Json& inNode)
{
	using Type = std::decay_t<T>;
	if constexpr (std::is_same_v<Type, std::string>)
	{
		return inNode.is_string();
	}
	else if constexpr (std::is_same_v<Type, bool>)
	{
		return inNode.is_boolean();
	}
	else if constexpr (std::is_unsigned_v<Type>)
	{
		return inNode.is_number_unsigned() || inNode.is_number_integer();
	}
	else if constexpr (std::is_floating_point_v<Type>)
	{
		return inNode.is_number_float() || inNode.is_number_integer() || inNode.is_number_unsigned();
	}
	else if constexpr (std::is_integral_v<Type>)
	{
		return inNode.is_number_integer() || inNode.is_number_unsigned();
	}
	else
	{
		chalet_assert(false, "json::isValid - type not implemented");
		return false;
	}
}

/*****************************************************************************/
template <typename T>
inline bool json::isValid(const Json& inNode, const char* inKey)
{
	if (/* !inNode.is_object() || */ !inNode.contains(inKey))
		return false;

	return json::isValid<T>(inNode[inKey]);
}

/*****************************************************************************/
inline bool json::isNull(const Json& inNode, const char* inKey)
{
	return inNode.contains(inKey) && inNode[inKey].is_null();
}
inline bool json::isArray(const Json& inNode, const char* inKey)
{
	return inNode.contains(inKey) && inNode[inKey].is_array();
}
inline bool json::isObject(const Json& inNode, const char* inKey)
{
	return inNode.contains(inKey) && inNode[inKey].is_object();
}

/*****************************************************************************/
template <typename T>
inline T json::get(const Json& inNode)
{
	if (json::isValid<T>(inNode))
		return inNode.template get<T>();

	return T{};
}

/*****************************************************************************/
template <typename T>
inline T json::get(const Json& inNode, const char* inKey)
{
	if (json::isValid<T>(inNode, inKey))
		return inNode[inKey].template get<T>();

	return T{};
}

/*****************************************************************************/
template <typename T>
inline bool json::assign(T& outVariable, const Json& inNode)
{
	if (!json::isValid<T>(inNode))
		return false;

	using Type = std::decay_t<T>;
	outVariable = inNode.template get<Type>();
	return true;
}

/*****************************************************************************/
template <typename T>
bool json::assign(T& outVariable, const Json& inNode, const char* inKey)
{
	if (!json::isValid<T>(inNode, inKey))
		return false;

	using Type = std::decay_t<T>;
	outVariable = inNode[inKey].template get<Type>();
	return true;
}

/*****************************************************************************/
inline bool json::isStringInvalidOrEmpty(const Json& inNode)
{
	if (!json::isValid<std::string>(inNode))
		return true;

	return inNode.get<std::string>().empty();
}
inline bool json::isStringInvalidOrEmpty(const Json& inNode, const char* inKey)
{
	if (!json::isValid<std::string>(inNode, inKey))
		return true;

	return inNode[inKey].get<std::string>().empty();
}

/*****************************************************************************/
template <typename T>
inline bool json::assignNodeIfEmpty(Json& outNode, const char* inKey, const T& inValue)
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

	return result;
}

/*****************************************************************************/
template <typename T>
inline bool json::assignNodeIfEmptyWithFallback(Json& outNode, const char* inKey, const std::optional<T>& inValue, const T& inFallbackValue)
{
	bool result = false;
	bool valid = json::isValid<T>(outNode, inKey) && !inValue.has_value();

	if (!valid)
	{
		if (inValue.has_value())
			outNode[inKey] = *inValue;
		else
			outNode[inKey] = inFallbackValue;

		result = true;
	}

	return result;
}

/*****************************************************************************/
inline bool json::assignNodeIfEmptyWithFallback(Json& outNode, const char* inKey, const std::string& inValue, const std::string& inFallbackValue)
{
	bool result = false;
	if (!outNode.contains(inKey) || !outNode[inKey].is_string())
	{
		outNode[inKey] = inFallbackValue;
		result = true;
	}

	auto value = outNode[inKey].get<std::string>();
	if (!inValue.empty() && inValue != value)
	{
		outNode[inKey] = inValue;
		result = true;
	}

	return result;
}

/*****************************************************************************/
inline bool json::assignNodeWithFallback(Json& outNode, const char* inKey, const std::string& inValue, const std::string& inFallbackValue)
{
	bool result = false;
	if (!outNode.contains(inKey) || !outNode[inKey].is_string())
	{
		outNode[inKey] = inFallbackValue;
		result = true;
	}

	auto value = outNode[inKey].get<std::string>();
	if (!inValue.empty() && !value.empty() && inValue != value)
	{
		outNode[inKey] = inValue;
		result = true;
	}

	return result;
}

/*****************************************************************************/
inline bool json::assignObjectNodeIfInvalid(Json& outNode, const char* inKey)
{
	if (!json::isObject(outNode, inKey))
	{
		outNode[inKey] = Json::object();
		return true;
	}

	return false;
}
inline bool json::assignObjectNodeIfInvalid(Json& outNode, const char* inKey, const Json& inObjectNode)
{
	if (!json::isObject(outNode, inKey))
	{
		outNode[inKey] = inObjectNode.is_object() ? inObjectNode : Json::object();
		return true;
	}

	return false;
}

/*****************************************************************************/
inline bool json::assignObjectNodeIfInvalidAndIncludeMissingPairs(Json& outNode, const char* inKey, const Json& inObjectNode)
{
	if (!json::isObject(outNode, inKey))
	{
		if (inObjectNode.is_object())
			outNode[inKey] = inObjectNode;
		else
			outNode[inKey] = Json::object();

		return true;
	}
	else
	{
		bool result = false;
		if (inObjectNode.is_object())
		{
			auto& tools = outNode[inKey];
			for (auto& [key, value] : inObjectNode.items())
			{
				if (!tools.contains(key))
				{
					tools[key] = value;
					result = true;
				}
			}
		}
		return result;
	}
}

/*****************************************************************************/
inline bool json::removeNode(Json& outNode, const char* inKey)
{
	if (outNode.is_object() && outNode.contains(inKey))
	{
		outNode.erase(inKey);
		return true;
	}
	return false;
}

/*****************************************************************************/
bool json::assignNodeFromDataType(Json& outNode, const char* inKey, const JsonDataType inType)
{
	if (!outNode.is_object())
		return false;

	if (outNode.contains(inKey))
	{
		if (outNode[inKey].type() == inType)
			return false;
	}

	outNode[inKey] = initializeDataType(inType);
	return true;
}

/*****************************************************************************/
Json json::initializeDataType(const JsonDataType inType)
{
	switch (inType)
	{
		case JsonDataType::object:
			return Json::object();

		case JsonDataType::array:
			return Json::array();

		case JsonDataType::string:
			return std::string();

		case JsonDataType::binary:
			return 0x0;

		case JsonDataType::boolean:
			return false;

		case JsonDataType::number_float:
			return 0.0f;

		case JsonDataType::number_integer:
			return 0;

		case JsonDataType::number_unsigned:
			return 0U;

		default:
			return Json();
	}
}

/*****************************************************************************/
inline std::string json::dump(const Json& inNode, int indent, char indentChar)
{
	constexpr bool ensureAscii = false;
	constexpr auto errorHandler = nlohmann::detail::error_handler_t::replace;
	return inNode.dump(indent, indentChar, ensureAscii, errorHandler);
}
}