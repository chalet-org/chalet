/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_JSON_HPP
#define CHALET_JSON_HPP

#if !defined(CHALET_EXCEPTIONS)
	#define JSON_NOEXCEPTION
#endif

#ifdef CHALET_MSVC
	// #pragma warning(push)
	// #pragma warning(disable : 4100)
#else
	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wpedantic"
	#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif

#include <nlohmann/json.hpp>

#include <nlohmann/json-schema.hpp>

#ifdef CHALET_MSVC
	// #pragma warning(pop)
#else
	#pragma GCC diagnostic pop
#endif

inline nlohmann::ordered_json operator"" _ojson(const char* s, std::size_t n)
{
	return nlohmann::ordered_json::parse(s, s + n);
}

namespace chalet
{
using Json = nlohmann::ordered_json;
using JsonDataType = nlohmann::detail::value_t;
namespace JsonSchema = nlohmann::json_schema;
using JsonSchemaError = JsonSchema::error_descriptor;
}

#endif // CHALET_JSON_HPP
