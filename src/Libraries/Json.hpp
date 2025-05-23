/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

// Exceptions will throw during schema construction otherwise
#if !defined(CHALET_EXCEPTIONS)
	#ifndef JSON_NOEXCEPTION
		#define JSON_NOEXCEPTION
	#endif
	#ifndef JSONSV_NOEXCEPTION
		#define JSONSV_NOEXCEPTION
	#endif
#endif

#if defined(CHALET_MSVC)
	#pragma warning(push)
	// #pragma warning(disable : 4100)
	#pragma warning(disable : 4996)
#else
	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wpedantic"
	#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif

#include <nlohmann/json-schema.hpp>
#include <nlohmann/json.hpp>

#if defined(CHALET_MSVC)
	#pragma warning(pop)
#else
	#pragma GCC diagnostic pop
#endif

inline nlohmann::ordered_json operator"" _ojson(const char* s, size_t n)
{
	return nlohmann::ordered_json::parse(s, s + n);
}

namespace chalet
{
using UJson = nlohmann::json;
using Json = nlohmann::ordered_json;
using JsonDataType = nlohmann::detail::value_t;
namespace JsonSchema = nlohmann::json_schema;
using JsonSchemaError = JsonSchema::error_descriptor;

// JSON helper functions
namespace json
{
template <typename T>
constexpr bool isValid(const Json& inNode);

template <typename T>
inline bool isValid(const Json& inNode, const char* inKey);

inline bool isNull(const Json& inNode, const char* inKey);
inline bool isArray(const Json& inNode, const char* inKey);
inline bool isObject(const Json& inNode, const char* inKey);

template <typename T>
inline T get(const Json& inNode);

template <typename T>
inline T get(const Json& inNode, const char* inKey);

template <typename T>
inline bool assign(T& outVariable, const Json& inNode);

template <typename T>
inline bool assign(T& outVariable, const Json& inNode, const char* inKey);

inline bool isStringInvalidOrEmpty(const Json& inNode);
inline bool isStringInvalidOrEmpty(const Json& inNode, const char* inKey);

inline std::string dump(const Json& inNode, int indent = -1, char indentChar = (char)32);
}
}

#include "Libraries/Json.inl"
