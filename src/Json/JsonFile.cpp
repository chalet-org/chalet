/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Json/JsonFile.hpp"

#include "Terminal/Commands.hpp"
#include "Json/JsonComments.hpp"
#include "Json/JsonValidator.hpp"

namespace chalet
{
/*****************************************************************************/
JsonFile::JsonFile(std::string inFilename) :
	m_filename(std::move(inFilename))
{
	load();
}

/*****************************************************************************/
void JsonFile::saveToFile(const Json& inJson, const std::string& outFilename)
{
	if (outFilename.empty())
		return;

	std::ofstream(outFilename) << inJson.dump(1, '\t') << std::endl;
}

/*****************************************************************************/
void JsonFile::load()
{
	chalet_assert(!m_filename.empty(), "JsonFile::load(): No file to load");
	json = JsonComments::parse(m_filename);
}

/*****************************************************************************/
void JsonFile::load(std::string inFilename)
{
	m_filename = std::move(inFilename);

	load();
}

/*****************************************************************************/
void JsonFile::save()
{
	if (m_filename.empty())
		return;

	JsonFile::saveToFile(json, m_filename);
}

/*****************************************************************************/
void JsonFile::setContents(Json&& inJson)
{
	json = std::move(inJson);
}

/*****************************************************************************/
const std::string& JsonFile::filename() const noexcept
{
	return m_filename;
}

/*****************************************************************************/
void JsonFile::makeNode(const std::string& inName, const JsonDataType inType)
{
	initializeDataType(json[inName], inType);
}

/*****************************************************************************/
bool JsonFile::validate(Json&& inSchemaJson)
{
	if (m_filename.empty())
		return false;

	JsonValidator validator(std::move(inSchemaJson), m_filename);

	// false if any errors
	if (!validator.validate(json))
	{
		// false if fatal error
		if (!validator.printErrors())
			return false;
	}

	return true;
}

/*****************************************************************************/
void JsonFile::initializeDataType(Json& inJson, const JsonDataType inType)
{
	if (inJson.type() == inType)
		return;

	switch (inType)
	{
		case JsonDataType::object:
			inJson = Json::object();
			break;

		case JsonDataType::array:
			inJson = Json::array();
			break;

		case JsonDataType::string:
			inJson = std::string();
			break;

		case JsonDataType::binary:
			inJson = 0x0;
			break;

		case JsonDataType::boolean:
			inJson = false;
			break;

		case JsonDataType::number_float:
			inJson = 0.0f;
			break;

		case JsonDataType::number_integer:
			inJson = 0;
			break;

		case JsonDataType::number_unsigned:
			inJson = 0U;
			break;

		default:
			break;
	}
}
}
