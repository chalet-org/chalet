/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_JSON_HPP
#define CHALET_JSON_HPP

#include <nlohmann/json.hpp>

#include <nlohmann/json-schema.hpp>

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
