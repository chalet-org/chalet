/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Json/JsonValidator.hpp"

#include "Terminal/Output.hpp"
#include "Utility/String.hpp"
#include "Json/JsonComments.hpp"

namespace chalet
{
using Validator = JsonSchema::json_validator;

/*****************************************************************************/
struct JsonValidator::Impl
{
	Validator validator;
};

/*****************************************************************************/
struct ErrorHandler : JsonSchema::error_handler
{
	explicit ErrorHandler(JsonValidationErrors& inErrors, const std::string& inFile) :
		m_errors(inErrors),
		m_file(inFile)
	{}

private:
	JsonValidationErrors& m_errors;
	const std::string& m_file;

	const std::string kRootKey{ "(root)" };

	std::string getValueFromDump(const std::string& inString);
	std::string getPropertyFromErrorMsg(const std::string& inString);
	std::string parseRawError(JsonValidationError& outError);
	void getValueWithTypCheck(std::any& data, std::string& outString) const;

	virtual void error(const nlohmann::json_pointer<nlohmann::json>& pointer, const nlohmann::json& instance, const JsonSchemaError type, std::any data) final;
};

/*****************************************************************************/
void ErrorHandler::error(const nlohmann::json_pointer<nlohmann::json>& pointer, const nlohmann::json& instance, const JsonSchemaError type, std::any data)
{
	JsonValidationError error;

	// check if we're in an array
	if (!pointer.empty())
	{
		if (pointer.back().find_first_not_of("0123456789") == std::string::npos)
		{
			// use the key of the array instead of the index
			error.key = pointer.parent_pointer().back();
		}
		else
		{
			error.key = pointer.back();
		}
	}

	error.classification = JsonErrorClassification::Fatal;
	error.typeName = instance.type_name();
	error.type = type;
	error.data = std::move(data);
	error.value = instance.dump();

	const auto treeReference = pointer.to_string();
	if (treeReference.size() > 0)
	{
		error.tree = String::split(treeReference.substr(1), "/");
	}

	error.message = parseRawError(error);

	if (!error.message.empty())
	{
		m_errors.push_back(error);
	}
}

/*****************************************************************************/
std::string ErrorHandler::getValueFromDump(const std::string& inString)
{
	return inString.substr(1, inString.size() - 2);
}

/*****************************************************************************/
std::string ErrorHandler::getPropertyFromErrorMsg(const std::string& inString)
{
	size_t start = inString.find_first_of('\'') + 1;
	size_t end = inString.find_last_of('\'');

	return inString.substr(start, end - start);
}

/*****************************************************************************/
void ErrorHandler::getValueWithTypCheck(std::any& data, std::string& outString) const
{
	if (std::any_cast<f64>(&data) || std::any_cast<f32>(&data))
	{
		auto val = std::any_cast<Json::number_float_t>(data);
		outString = std::to_string(val);
	}
	else if (std::any_cast<i64>(&data) || std::any_cast<i32>(&data))
	{
		auto val = std::any_cast<Json::number_integer_t>(data);
		outString = std::to_string(val);
	}
	else if (std::any_cast<u64>(&data) || std::any_cast<u32>(&data))
	{
		auto val = std::any_cast<Json::number_unsigned_t>(data);
		outString = std::to_string(val);
	}
	else
	{
		outString = "unknown";
	}
}

/*****************************************************************************/
std::string ErrorHandler::parseRawError(JsonValidationError& outError)
{
	auto& data = outError.data;

	/*
		// Leftover from old error handler (wtf was this?)

		if (String::startsWith("Value", outError.messageRaw))
		{
			return fmt::format("An invalid value was found in '{}'. {}", outError.key, outError.messageRaw);
		}
	*/

	const std::string& parentKey = outError.key.empty() ? kRootKey : outError.key;

	switch (outError.type)
	{
		case JsonSchemaError::schema_ref_unresolved:
			return fmt::format("Unresolved or freed schema-reference {}", std::any_cast<std::string>(data));

		case JsonSchemaError::no_root_schema_set:
			return "No root schema has yet been set for validating an instance.";

		case JsonSchemaError::logical_not:
			return fmt::format("The subschema for '{}' is required to not match (per 'not'), but in this cased matched.", parentKey);

		case JsonSchemaError::logical_combination_any_of:
			return fmt::format("At least one of the subschemas are required for '{}' (per 'anyOf'), but none of them matched.", parentKey);

		case JsonSchemaError::logical_combination_all_of:
			return fmt::format("All of the subschemas are required for '{}' (per 'allOf'), but at least one of them did not match.", parentKey);

		case JsonSchemaError::logical_combination_one_of:
			return fmt::format("One of the subschemas are required for '{}' (per 'oneOf'), but none matched.", parentKey);

		case JsonSchemaError::type_instance_unexpected_type: {
			if (String::equals(kRootKey, parentKey) && String::equals("null", outError.typeName))
			{
				// There should also be a JSON exception, and a generic error message that prints,
				//   so this extra one is just confusing
				return std::string();
			}
			else
			{
				return fmt::format("An invalid value was found in '{}'. Found {}", parentKey, outError.typeName);
			}
		}

		case JsonSchemaError::type_instance_not_found_in_required_enum:
			return std::string(); // not currently handled - can throw a false positive with unresolved const & enum comparisons

		case JsonSchemaError::type_instance_not_const:
			return std::string(); // not currently handled - can throw a false positive with unresolved const & enum comparisons

		case JsonSchemaError::string_min_length: {
			const size_t min_length = std::any_cast<size_t>(data);
			return fmt::format("A {} in '{}' is shorter than the minimum length of {}", outError.typeName, parentKey, min_length);
		}

		case JsonSchemaError::string_max_length: {
			const size_t max_length = std::any_cast<size_t>(data);
			return fmt::format("A {} in '{}' is longer than the maximum length of {}", outError.typeName, parentKey, max_length);
		}

		case JsonSchemaError::string_content_checker_not_provided: {
			auto sub_data = std::any_cast<std::pair<std::string, std::string>>(data);
			return fmt::format("A content checker was not provided but a contentEncoding or contentMediaType for this string have been present: '{}' '{}'", sub_data.first, sub_data.second);
		}

		case JsonSchemaError::string_content_checker_failed:
			return "Content-checking failed: " + std::any_cast<std::string>(data);

		case JsonSchemaError::string_expected_found_binary_data:
			return "Expected string, but get binary data";

		case JsonSchemaError::string_regex_pattern_mismatch: {
			auto pattern = std::any_cast<std::string>(data);
			std::string value = getValueFromDump(outError.value);

			return fmt::format("An invalid value was found in '{}': '{}'. Expected the pattern '{}'", parentKey, value, pattern);
		}

		case JsonSchemaError::string_format_checker_not_provided:
			return fmt::format("A format checker was not provided but a format keyword for this string is present: {}", std::any_cast<std::string>(data));

		case JsonSchemaError::string_format_checker_failed:
			return fmt::format("Format-checking failed: {}", std::any_cast<std::string>(data));

		case JsonSchemaError::numeric_multiple_of: {
			std::string multiple;
			getValueWithTypCheck(data, multiple);
			return fmt::format("Instance is not a multiple of {}", multiple);
		}

		case JsonSchemaError::numeric_exceeds_maximum: {
			std::string maximum;
			getValueWithTypCheck(data, maximum);
			return fmt::format("Instance exceeds maximum of {}", maximum);
		}

		case JsonSchemaError::numeric_below_minimum: {
			std::string minimum;
			getValueWithTypCheck(data, minimum);
			return fmt::format("Instance is below minimum of {}", minimum);
		}

		case JsonSchemaError::null_found_non_null:
			return "Expected to be null";

			// case JsonSchemaError::boolean_false_schema_required_empty_array:
			// 	return "false-schema required empty array";

		case JsonSchemaError::boolean_invalid_per_false_schema:
			return "Not allowed.";

		case JsonSchemaError::required_property_not_found: {
			auto property = std::any_cast<std::string>(data);

			return fmt::format("The property '{}' is required by {} '{}', but was not found.", property, outError.typeName, parentKey);
		}

		case JsonSchemaError::object_too_many_properties:
			return "Too many properties";

		case JsonSchemaError::object_too_few_properties:
			return "Too few properties";

		case JsonSchemaError::object_required_property_not_found: {
			auto property = std::any_cast<std::string>(data);

			return fmt::format("The property '{}' is required by {} '{}', but was not found.", property, outError.typeName, parentKey);
		}

		case JsonSchemaError::object_additional_property_failed: {
			const auto msg = std::any_cast<std::tuple<JsonSchemaError, std::any, std::string>>(data);

			JsonValidationError subError = outError;
			subError.type = std::any_cast<JsonSchemaError>(std::get<0>(msg));
			subError.data = std::move(std::get<1>(msg));
			const auto& key = std::get<2>(msg);

			return fmt::format("The object '{}' contains an unknown property '{}': {}", parentKey, key, parseRawError(subError));
		}

		case JsonSchemaError::array_required_not_empty:
			return fmt::format("Array property '{}' was empty, but requires at least one item", parentKey);

		case JsonSchemaError::array_too_many_items:
			return fmt::format("Array property '{}' has too many items", parentKey);

		case JsonSchemaError::array_too_few_items:
			return fmt::format("Array property '{}' has too few items", parentKey);

		case JsonSchemaError::array_items_must_be_unique:
			return "Items have to be unique for this array";

		case JsonSchemaError::array_does_not_contain_required_element_per_contains:
			return "Array does not contain required element as per 'contains'";

		case JsonSchemaError::none:
		default:
			break;
	}

	Diagnostic::error(fmt::format("{}: Schema failed validation for '{}' (expected {}). Unhandled Json type: {}.", m_file, parentKey, outError.typeName, static_cast<std::underlying_type<JsonSchemaError>::type>(outError.type)));

	return std::string();
}

/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
JsonValidator::JsonValidator() :
	m_impl(std::make_unique<Impl>())
{
}

/*****************************************************************************/
JsonValidator::~JsonValidator() = default;

/*****************************************************************************/
bool JsonValidator::setSchema(const Json& inSchema)
{
	CHALET_TRY
	{
		m_impl->validator.set_root_schema(inSchema);
		return true;
	}
	CHALET_CATCH(const std::exception& err)
	{
		CHALET_EXCEPT_ERROR(err.what());
		return false;
	}
}

/*****************************************************************************/
bool JsonValidator::validate(const Json& inJsonContent, const std::string& inFile, JsonValidationErrors& errors)
{
	CHALET_TRY
	{
		if (!inJsonContent.is_object())
		{
			Diagnostic::error("{}: Root node must be an object.", inFile);
			return false;
		}

		ErrorHandler errorHandler{ errors, inFile };
		m_impl->validator.validate(inJsonContent, errorHandler);

		return errors.size() == 0;
	}
	CHALET_CATCH(const std::exception& err)
	{
		CHALET_EXCEPT_ERROR(err.what());
		return false;
	}
}

/*****************************************************************************/
bool JsonValidator::printErrors(JsonValidationErrors& errors)
{
	if (errors.size() == 0)
		return true;

	for (auto& error : errors)
	{
		// Pass them to the primary error handler
		String::replaceAll(error.message, '{', "{{");
		String::replaceAll(error.message, '}', "}}");
		Diagnostic::error(error.message);
		// Diagnostic::error(fmt::format("   {}", error.message));
	}

	return false;
}

}
