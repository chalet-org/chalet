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
	else if constexpr (std::is_integral_v<Type>)
	{
		return inNode.is_number_integer();
	}
	else if constexpr (std::is_unsigned_v<Type>)
	{
		return inNode.is_number_unsigned();
	}
	else if constexpr (std::is_floating_point_v<Type>)
	{
		return inNode.is_number_float();
	}
	else if constexpr (std::is_same_v<Type, bool>)
	{
		return inNode.is_boolean();
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

	return json::isValid<T>(inNode.at(inKey));
}

/*****************************************************************************/
inline bool json::isNull(const Json& inNode, const char* inKey)
{
	return inNode.contains(inKey) && inNode.at(inKey).is_null();
}
inline bool json::isArray(const Json& inNode, const char* inKey)
{
	return inNode.contains(inKey) && inNode.at(inKey).is_array();
}
inline bool json::isObject(const Json& inNode, const char* inKey)
{
	return inNode.contains(inKey) && inNode.at(inKey).is_object();
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
		return inNode.at(inKey).template get<T>();

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
	outVariable = inNode.at(inKey).template get<Type>();
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

	return inNode.at(inKey).get<std::string>().empty();
}
}