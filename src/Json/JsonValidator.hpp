/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_JSON_VALIDATOR_HPP
#define CHALET_JSON_VALIDATOR_HPP

#include "Libraries/Json.hpp"

#include "Json/JsonValidationError.hpp"

namespace chalet
{
struct JsonValidator
{
	using ValidationErrors = std::vector<JsonValidationError>;

	explicit JsonValidator(Json&& inSchema, const std::string& inFile);
	~JsonValidator();

	bool validate(const Json& inJsonContent);
	const ValidationErrors& errors() const noexcept;

	bool printErrors();

private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;

	ValidationErrors m_errors;
	const std::string& m_file;
};
}

#endif // CHALET_JSON_VALIDATOR_HPP
