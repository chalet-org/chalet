/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include "Libraries/Json.hpp"

#include "Json/JsonValidationError.hpp"

namespace chalet
{
struct JsonValidator
{
	JsonValidator();
	CHALET_DISALLOW_COPY_MOVE(JsonValidator);
	~JsonValidator();

	bool setSchema(const Json& inSchema);

	bool validate(const Json& inJsonContent, const std::string& inFile, JsonValidationErrors& errors);

	bool printErrors(JsonValidationErrors& errors);

private:
	struct Impl;
	Unique<Impl> m_impl;
};
}
