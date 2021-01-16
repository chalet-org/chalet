/*
 * JSON schema validator for JSON for modern C++
 *
 * Copyright (c) 2016-2019 Patrick Boettcher <p@yai.se>.
 *
 * SPDX-License-Identifier: MIT
 *
 */
#ifndef NLOHMANN_JSON_SCHEMA_HPP__
#define NLOHMANN_JSON_SCHEMA_HPP__

#ifdef _WIN32
#	if defined(JSON_SCHEMA_VALIDATOR_EXPORTS)
#		define JSON_SCHEMA_VALIDATOR_API __declspec(dllexport)
#	elif defined(JSON_SCHEMA_VALIDATOR_IMPORTS)
#		define JSON_SCHEMA_VALIDATOR_API __declspec(dllimport)
#	else
#		define JSON_SCHEMA_VALIDATOR_API
#	endif
#else
#	define JSON_SCHEMA_VALIDATOR_API
#endif

#include <nlohmann/json.hpp>

#ifdef NLOHMANN_JSON_VERSION_MAJOR
#	if (NLOHMANN_JSON_VERSION_MAJOR * 10000 + NLOHMANN_JSON_VERSION_MINOR * 100 + NLOHMANN_JSON_VERSION_PATCH) < 30800
#		error "Please use this library with NLohmann's JSON version 3.8.0 or higher"
#	endif
#else
#	error "expected existing NLOHMANN_JSON_VERSION_MAJOR preproc variable, please update to NLohmann's JSON 3.8.0"
#endif

#include <any>

// make yourself a home - welcome to nlohmann's namespace
namespace nlohmann
{

// A class representing a JSON-URI for schemas derived from
// section 8 of JSON Schema: A Media Type for Describing JSON Documents
// draft-wright-json-schema-00
//
// New URIs can be derived from it using the derive()-method.
// This is useful for resolving refs or subschema-IDs in json-schemas.
//
// This is done implement the requirements described in section 8.2.
//
class JSON_SCHEMA_VALIDATOR_API json_uri
{
	std::string urn_;

	std::string scheme_;
	std::string authority_;
	std::string path_;

	json::json_pointer pointer_; // fragment part if JSON-Pointer
	std::string identifier_;     // fragment part if Locatation Independent ID

protected:
	// decodes a JSON uri and replaces all or part of the currently stored values
	void update(const std::string &uri);

	std::tuple<std::string, std::string, std::string, std::string, std::string> as_tuple() const
	{
		return std::make_tuple(urn_, scheme_, authority_, path_, identifier_ != "" ? identifier_ : pointer_);
	}

public:
	json_uri(const std::string &uri)
	{
		update(uri);
	}

	const std::string &scheme() const { return scheme_; }
	const std::string &authority() const { return authority_; }
	const std::string &path() const { return path_; }

	const json::json_pointer &pointer() const { return pointer_; }
	const std::string &identifier() const { return identifier_; }

	std::string fragment() const
	{
		if (identifier_ == "")
			return pointer_;
		else
			return identifier_;
	}

	std::string url() const { return location(); }
	std::string location() const;

	static std::string escape(const std::string &);

	// create a new json_uri based in this one and the given uri
	// resolves relative changes (pathes or pointers) and resets part if proto or hostname changes
	json_uri derive(const std::string &uri) const
	{
		json_uri u = *this;
		u.update(uri);
		return u;
	}

	// append a pointer-field to the pointer-part of this uri
	json_uri append(const std::string &field) const
	{
		if (identifier_ != "")
			return *this;

		json_uri u = *this;
		u.pointer_ /= field;
		return u;
	}

	std::string to_string() const;

	friend bool operator<(const json_uri &l, const json_uri &r)
	{
		return l.as_tuple() < r.as_tuple();
	}

	friend bool operator==(const json_uri &l, const json_uri &r)
	{
		return l.as_tuple() == r.as_tuple();
	}

	friend std::ostream &operator<<(std::ostream &os, const json_uri &u);
};

namespace json_schema
{

extern json draft7_schema_builtin;

typedef std::function<void(const json_uri & /*id*/, json & /*value*/)> schema_loader;
typedef std::function<void(const std::string & /*format*/, const std::string & /*value*/)> format_checker;
typedef std::function<void(const std::string & /*contentEncoding*/, const std::string & /*contentMediaType*/, const json & /*instance*/)> content_checker;

enum class error_descriptor : std::uint8_t {
	none,
	schema_ref_unresolved,
	no_root_schema_set,
	logical_not,
	logical_combination,
	logical_combination_all_of,
	logical_combination_any_of,
	logical_combination_one_of,
	type_instance_unexpected_type,
	type_instance_not_found_in_required_enum,
	type_instance_not_const,
	string_min_length,
	string_max_length,
	string_content_checker_not_provided,
	string_content_checker_failed,
	string_expected_found_binary_data,
	string_regex_pattern_mismatch,
	string_format_checker_not_provided,
	string_format_checker_failed,
	numeric_multiple_of,
	numeric_exceeds_maximum,
	numeric_below_minimum,
	null_found_non_null,
	// boolean_false_schema_required_empty_array,
	boolean_invalid_per_false_schema,
	required_property_not_found,
	object_too_many_properties,
	object_too_few_properties,
	object_required_property_not_found,
	object_additional_property_failed,
	array_too_many_items,
	array_too_few_items,
	array_items_must_be_unique,
	array_does_not_contain_required_element_per_contains,
};

// Interface for validation error handlers
class JSON_SCHEMA_VALIDATOR_API error_handler
{
public:
	virtual ~error_handler() {}
	virtual void error(const json::json_pointer & /*ptr*/, const json & /*instance*/, const error_descriptor /*type*/, std::any /*data*/ = std::any{}) = 0;
};

class JSON_SCHEMA_VALIDATOR_API basic_error_handler : public error_handler
{
	bool error_{false};

public:
	void error(const json::json_pointer & /*ptr*/, const json & /*instance*/, const error_descriptor /*type*/, std::any /*data*/ = std::any{}) override
	{
		error_ = true;
	}

	virtual void reset() { error_ = false; }
	operator bool() const { return error_; }
};

/**
 * Checks validity of JSON schema built-in string format specifiers like 'date-time', 'ipv4', ...
 */
void default_string_format_check(const std::string &format, const std::string &value);

/**
 * Takes the error type and data and returns a string
 */
std::string error_descriptor_type_to_string(const error_descriptor type, const std::any &data);

class root_schema;

class JSON_SCHEMA_VALIDATOR_API json_validator
{
	std::unique_ptr<root_schema> root_;

public:
	json_validator(schema_loader = nullptr, format_checker = nullptr, content_checker = nullptr);

	json_validator(const json &, schema_loader = nullptr, format_checker = nullptr, content_checker = nullptr);
	json_validator(json &&, schema_loader = nullptr, format_checker = nullptr, content_checker = nullptr);

	json_validator(json_validator &&);
	json_validator &operator=(json_validator &&);

	json_validator(json_validator const &) = delete;
	json_validator &operator=(json_validator const &) = delete;

	~json_validator();

	// insert and set the root-schema
	void set_root_schema(const json &);
	void set_root_schema(json &&);

	// validate a json-document based on the root-schema
	json validate(const json &) const;

	// validate a json-document based on the root-schema with a custom error-handler
	json validate(const json &, error_handler &) const;
};

} // namespace json_schema
} // namespace nlohmann

#endif /* NLOHMANN_JSON_SCHEMA_HPP__ */
