/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_JSON_VALIDATION_ERROR_HPP
#define CHALET_JSON_VALIDATION_ERROR_HPP

#include "Libraries/Json.hpp"

#include "Json/JsonErrorClassification.hpp"

namespace chalet
{
struct JsonValidationError
{
	std::string key;
	std::string value;
	std::string typeName;
	std::string message;
	StringList tree;
	std::any data;
	JsonSchemaError type = JsonSchemaError::none;
	JsonErrorClassification classification = JsonErrorClassification::None;
};
using JsonValidationErrors = std::vector<JsonValidationError>;
}

#endif // CHALET_JSON_VALIDATION_ERROR_HPP
