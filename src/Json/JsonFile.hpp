/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_JSON_FILE_HPP
#define CHALET_JSON_FILE_HPP

#include "Libraries/Json.hpp"

namespace chalet
{
struct JsonFile
{
	JsonFile() = default;
	explicit JsonFile(std::string inFilename);

	static void saveToFile(const Json& inJson, const std::string& outFilename);

	void load();
	void load(std::string inFilename);
	void save();

	void setContents(Json&& inJson);

	const std::string& filename() const noexcept;
	void makeNode(const std::string& inName, const JsonDataType inType);

	bool validate(Json&& inSchemaJson);

	Json json;

private:
	void initializeDataType(Json& inJson, const JsonDataType inType);

	std::string m_filename;
};
}

#endif // CHALET_JSON_FILE_HPP
