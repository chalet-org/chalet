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
	explicit ErrorHandler(JsonValidator::ValidationErrors& inErrors, const std::string& inFile) :
		m_errors(inErrors),
		m_file(inFile)
	{}

private:
	JsonValidator::ValidationErrors& m_errors;
	const std::string& m_file;

	const std::string kRootKey{ "(root)" };

	std::string getValueFromDump(const std::string& inString);
	std::string getPropertyFromErrorMsg(const std::string& inString);
	std::string parseRawError(JsonValidationError& outError);

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

	m_errors.push_back(error);
}

/*****************************************************************************/
std::string ErrorHandler::getValueFromDump(const std::string& inString)
{
	return inString.substr(1, inString.size() - 2);
}

/*****************************************************************************/
std::string ErrorHandler::getPropertyFromErrorMsg(const std::string& inString)
{
	std::size_t start = inString.find_first_of('\'') + 1;
	std::size_t end = inString.find_last_of('\'');

	return inString.substr(start, end - start);
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

	// LOG("outError.type: ", static_cast<int>(outError.type));

	const std::string& parentKey = outError.key.empty() ? kRootKey : outError.key;

	switch (outError.type)
	{
		case JsonSchemaError::schema_ref_unresolved:
			return "Unresolved or freed schema-reference " + std::any_cast<std::string>(data);

		case JsonSchemaError::no_root_schema_set:
			return "No root schema has yet been set for validating an instance";

		case JsonSchemaError::logical_not:
			return "The subschema has succeeded, but it is required to not validate";

		case JsonSchemaError::logical_combination: {
			auto msg = fmt::format("Invalid value type found in: {}", String::join(outError.tree, '.'));
			return msg;
		}

		case JsonSchemaError::logical_combination_all_of: {
			const auto msg = std::any_cast<std::pair<JsonSchemaError, std::any>>(data);

			JsonValidationError subError = outError;
			subError.type = msg.first;
			subError.data = std::move(msg.second);
			return "At least one subschema has failed, but all of them are required to validate - " + parseRawError(subError);
		}

		case JsonSchemaError::logical_combination_any_of:
			return std::string(); // not currently handled

		case JsonSchemaError::logical_combination_one_of:
			return "More than one subschema has succeeded, but exactly one of them is required to validate";

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

		case JsonSchemaError::type_instance_not_found_in_required_enum: {
			return fmt::format("An invalid value was found in '{}'. Expected string enum", parentKey);
		}

		case JsonSchemaError::type_instance_not_const:
			return "Instance not const";

		case JsonSchemaError::string_min_length: {
			const std::size_t min_length = std::any_cast<std::size_t>(data);
			return "Instance is too short as per minLength:" + std::to_string(min_length);
		}

		case JsonSchemaError::string_max_length: {
			const std::size_t max_length = std::any_cast<std::size_t>(data);
			return "Instance is too long as per maxLength:" + std::to_string(max_length);
		}

		case JsonSchemaError::string_content_checker_not_provided: {
			auto sub_data = std::any_cast<std::pair<std::string, std::string>>(data);
			return "A content checker was not provided but a contentEncoding or contentMediaType for this string have been present: '" + sub_data.first + "' '" + sub_data.second + '\'';
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
			return "A format checker was not provided but a format keyword for this string is present: " + std::any_cast<std::string>(data);

		case JsonSchemaError::string_format_checker_failed:
			return "Format-checking failed: " + std::any_cast<std::string>(data);

		case JsonSchemaError::numeric_multiple_of: {
			auto multiple = std::any_cast<Json::number_float_t>(data);
			return "Instance is not a multiple of " + std::to_string(multiple);
		}

		case JsonSchemaError::numeric_exceeds_maximum: {
			auto maximum = std::any_cast<Json::number_float_t>(data);
			return "Instance exceeds maximum of " + std::to_string(maximum);
		}

		case JsonSchemaError::numeric_below_minimum: {
			auto minimum = std::any_cast<Json::number_float_t>(data);
			return "Instance is below minimum of " + std::to_string(minimum);
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

			return fmt::format("Additional property '{}' found in '{}' object: {}", key, parentKey, parseRawError(subError));
		}

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

	Diagnostic::error("{}: Schema failed validation for '{}' (expected {}). Unhandled Json type: {}.", m_file, parentKey, outError.typeName, static_cast<std::underlying_type<JsonSchemaError>::type>(outError.type));

	return std::string();
}

/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
JsonValidator::JsonValidator(const std::string& inFile) :
	m_impl(std::make_unique<Impl>()),
	m_file(inFile)
{
}

/*****************************************************************************/
JsonValidator::~JsonValidator() = default;

/*****************************************************************************/
bool JsonValidator::setSchema(Json&& inSchema)
{
	CHALET_TRY
	{
		m_impl->validator.set_root_schema(std::move(inSchema));
		return true;
	}
	CHALET_CATCH(const std::exception& err)
	{
		CHALET_EXCEPT_ERROR(err.what());
		return false;
	}
}

/*****************************************************************************/
bool JsonValidator::validate(const Json& inJsonContent)
{
	CHALET_TRY
	{
		if (!inJsonContent.is_object())
		{
			Diagnostic::error("{}: Root node must be an object.", m_file);
			return false;
		}

		ErrorHandler errorHandler{ m_errors, m_file };
		m_impl->validator.validate(inJsonContent, errorHandler);

		return m_errors.size() == 0;
	}
	CHALET_CATCH(const std::exception& err)
	{
		CHALET_EXCEPT_ERROR(err.what());
		return false;
	}
}

/*****************************************************************************/
bool JsonValidator::printErrors()
{
	if (m_errors.size() == 0)
		return true;

	for (auto& error : m_errors)
	{
		if (error.message.empty())
			continue;

		// Pass them to the primary error handler
		String::replaceAll(error.message, '{', "{{");
		String::replaceAll(error.message, '}', "}}");
		Diagnostic::error("{}", error.message);
	}

	return false;
}

/*****************************************************************************/
const JsonValidator::ValidationErrors& JsonValidator::errors() const noexcept
{
	return m_errors;
}
}
