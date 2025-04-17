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
	void getValueWithTypeCheck(std::any& data, std::string& outString) const;

	virtual void error(const nlohmann::json_pointer<nlohmann::json>& pointer, const nlohmann::json& instance, const JsonSchemaError type, std::any data) final;
};

/*****************************************************************************/
void ErrorHandler::error(const nlohmann::json_pointer<nlohmann::json>& pointer, const nlohmann::json& instance, const JsonSchemaError type, std::any data)
{
	// ignore these
	if (type == JsonSchemaError::type_instance_not_found_in_required_enum || type == JsonSchemaError::type_instance_not_const)
		return;

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
	error.value = json::dump(instance);

	const auto treeReference = pointer.to_string();
	if (treeReference.size() > 0)
	{
		error.tree = String::split(treeReference.substr(1), "/");
	}

	error.message = parseRawError(error);

	bool isCombination = error.type == JsonSchemaError::logical_combination_all_of
		|| error.type == JsonSchemaError::logical_combination_any_of
		|| error.type == JsonSchemaError::logical_combination_one_of;

	if (!error.message.empty())
	{
		for (auto& e : m_errors)
		{
			bool localIsCombination = e.type == JsonSchemaError::logical_combination_all_of
				|| e.type == JsonSchemaError::logical_combination_any_of
				|| e.type == JsonSchemaError::logical_combination_one_of;

			if (isCombination && localIsCombination)
			{
				// skip additional errors from logical_combination types
				//   they come from the parent nodes, and it's really confusing...
				//
				return;
			}
			else if (e.type == JsonSchemaError::type_instance_unexpected_type && e.value == error.value)
			{
				// Unexpected type messages are useless if there are other errors for the same node
				e = error;
				return;
			}
		}

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
void ErrorHandler::getValueWithTypeCheck(std::any& data, std::string& outString) const
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

	const std::string& key = outError.key.empty() ? kRootKey : outError.key;

	switch (outError.type)
	{
		case JsonSchemaError::schema_ref_unresolved:
			return fmt::format("Unresolved or freed schema-reference {}", std::any_cast<std::string>(data));

		case JsonSchemaError::no_root_schema_set:
			return "No root schema has yet been set for validating an instance.";

		case JsonSchemaError::logical_not:
			return fmt::format("The '{}' property is required to not match a particular schema, but in this cased did.", key);

		case JsonSchemaError::logical_combination_any_of:
			return fmt::format("The '{}' property failed to match any of its required subschemas.", key);

		case JsonSchemaError::logical_combination_all_of:
			return fmt::format("The '{}' property failed to match all of its required subschemas.", key);

		case JsonSchemaError::logical_combination_one_of:
			return fmt::format("The '{}' property failed to match one of its required subschemas.", key);

		case JsonSchemaError::type_instance_unexpected_type: {
			if (String::equals(kRootKey, key) && String::equals("null", outError.typeName))
			{
				// There should also be a JSON exception, and a generic error message that prints,
				//   so this extra one is just confusing
				return std::string();
			}
			else
			{
				return fmt::format("An invalid type was found for '{}'. Found {}", key, outError.typeName);
			}
		}

		case JsonSchemaError::type_instance_not_found_in_required_enum:
			return std::string(); // not currently handled - can throw a false positive with unresolved const & enum comparisons

		case JsonSchemaError::type_instance_not_const:
			return std::string(); // not currently handled - can throw a false positive with unresolved const & enum comparisons

		case JsonSchemaError::string_min_length: {
			const size_t min_length = std::any_cast<size_t>(data);
			return fmt::format("A {} in '{}' is shorter than the minimum length of {}.", outError.typeName, key, min_length);
		}

		case JsonSchemaError::string_max_length: {
			const size_t max_length = std::any_cast<size_t>(data);
			return fmt::format("A {} in '{}' is longer than the maximum length of {}.", outError.typeName, key, max_length);
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

			return fmt::format("An invalid value was found in '{}': '{}'. Expected the pattern '{}'", key, value, pattern);
		}

		case JsonSchemaError::string_format_checker_not_provided:
			return fmt::format("A format checker was not provided but a format keyword for this string is present: {}", std::any_cast<std::string>(data));

		case JsonSchemaError::string_format_checker_failed:
			return fmt::format("Format-checking failed: {}", std::any_cast<std::string>(data));

		case JsonSchemaError::numeric_multiple_of: {
			std::string multiple;
			getValueWithTypeCheck(data, multiple);
			return fmt::format("Number is not a multiple of {}.", multiple);
		}

		case JsonSchemaError::numeric_exceeds_maximum: {
			std::string maximum;
			getValueWithTypeCheck(data, maximum);
			return fmt::format("Number exceeds maximum of {}.", maximum);
		}

		case JsonSchemaError::numeric_below_minimum: {
			std::string minimum;
			getValueWithTypeCheck(data, minimum);
			return fmt::format("Number is below minimum of {}.", minimum);
		}

		case JsonSchemaError::null_found_non_null:
			return fmt::format("Expected the type for '{}' to be null.", key);

			// case JsonSchemaError::boolean_false_schema_required_empty_array:
			// 	return "false-schema required empty array";

		case JsonSchemaError::boolean_invalid_per_false_schema:
			return "Not allowed.";

		case JsonSchemaError::required_property_not_found: {
			auto property = std::any_cast<std::string>(data);

			return fmt::format("The property '{}' is required by {} '{}', but was not found.", property, outError.typeName, key);
		}

		case JsonSchemaError::object_too_many_properties:
			return fmt::format("The '{}' object contains too many properties.", key);

		case JsonSchemaError::object_too_few_properties:
			return fmt::format("The '{}' object contains too few properties.", key);

		case JsonSchemaError::object_required_property_not_found: {
			auto property = std::any_cast<std::string>(data);

			return fmt::format("The property '{}' is required by {} '{}', but was not found.", property, outError.typeName, key);
		}

		case JsonSchemaError::object_additional_property_failed: {
			const auto msg = std::any_cast<std::tuple<JsonSchemaError, std::any, std::string>>(data);

			JsonValidationError subError = outError;
			subError.type = std::any_cast<JsonSchemaError>(std::get<0>(msg));
			subError.data = std::move(std::get<1>(msg));
			const auto& key2 = std::get<2>(msg);

			return fmt::format("The '{}' object contains an unknown property '{}': {}", key, key2, parseRawError(subError));
		}

		case JsonSchemaError::array_required_not_empty:
			return fmt::format("The '{}' array was empty, but requires at least one item.", key);

		case JsonSchemaError::array_too_many_items:
			return fmt::format("The '{}' array has too many items.", key);

		case JsonSchemaError::array_too_few_items:
			return fmt::format("The '{}' array has too few items.", key);

		case JsonSchemaError::array_items_must_be_unique:
			return fmt::format("The '{}' array must have unique items, but duplicates were found.", key);

		case JsonSchemaError::array_does_not_contain_required_element_per_contains:
			return fmt::format("The '{}' array does not contain required element as per 'contains'.", key);

		case JsonSchemaError::none:
		default:
			break;
	}

	Diagnostic::error(fmt::format("{}: Schema failed validation for '{}' (expected {}). Unhandled Json type: {}.", m_file, key, outError.typeName, static_cast<std::underlying_type<JsonSchemaError>::type>(outError.type)));

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

		return errors.empty();
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
	if (errors.empty())
		return true;

	for (auto it = errors.rbegin(); it != errors.rend(); it++)
	{
		auto& error = *it;
		// Pass them to the primary error handler
		String::replaceAll(error.message, '{', "{{");
		String::replaceAll(error.message, '}', "}}");
		Diagnostic::error(error.message);
		// Diagnostic::error(fmt::format("   {}", error.message));
	}

	return false;
}

}
