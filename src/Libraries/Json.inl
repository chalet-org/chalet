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
inline std::string json::dump(const Json& inNode, int indent, char indentChar)
{
	constexpr bool ensureAscii = false;
	constexpr auto errorHandler = nlohmann::detail::error_handler_t::strict;
	return inNode.dump(indent, indentChar, ensureAscii, errorHandler);
}
}