/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_JSON_HPP
#define CHALET_JSON_HPP

#include <nlohmann/json.hpp>

#include "json-schema-validator/nlohmann/json-schema.hpp"

namespace chalet
{
using Json = nlohmann::ordered_json;
using JsonDataType = nlohmann::detail::value_t;
namespace JsonSchema = nlohmann::json_schema;
using JsonSchemaError = JsonSchema::error_descriptor;
}

#endif // CHALET_JSON_HPP
