/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Json/JsonFile.hpp"

#include "Terminal/Commands.hpp"
#include "Terminal/Output.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"
#include "Json/JsonComments.hpp"
#include "Json/JsonValidator.hpp"

namespace chalet
{
/*****************************************************************************/
JsonFile::JsonFile(std::string inFilename) :
	m_filename(std::move(inFilename))
{
}

/*****************************************************************************/
bool JsonFile::saveToFile(const Json& inJson, const std::string& outFilename, const int inIndent)
{
	if (outFilename.empty())
		return false;

	const auto folder = String::getPathFolder(outFilename);
	if (!folder.empty() && !Commands::pathExists(folder))
	{
		if (!Commands::makeDirectory(folder))
			return false;
	}

	if ((inIndent < -1 || inIndent > 4) || inIndent == 1)
		std::ofstream(outFilename) << inJson.dump(1, '\t') << std::endl;
	else
		std::ofstream(outFilename) << inJson.dump(inIndent) << std::endl;

	return true;
}

/*****************************************************************************/
bool JsonFile::load(const bool inError)
{
	chalet_assert(!m_filename.empty(), "JsonFile::load(): No file to load");
	return JsonComments::parse(json, m_filename, inError);
}

/*****************************************************************************/
bool JsonFile::load(std::string inFilename, const bool inError)
{
	m_filename = std::move(inFilename);

	return load(inError);
}

/*****************************************************************************/
bool JsonFile::save(const int inIndent)
{
	if (!m_filename.empty() && m_dirty)
	{
		return JsonFile::saveToFile(json, m_filename, inIndent);
	}

	// if there's nothing to save, we don't care
	return true;
}

/*****************************************************************************/
bool JsonFile::dirty() const noexcept
{
	return m_dirty;
}
void JsonFile::setDirty(const bool inValue) noexcept
{
	m_dirty = inValue;
}

/*****************************************************************************/
void JsonFile::resetAndSave()
{
	json = Json::object();
	setDirty(true);
	save();
}

/*****************************************************************************/
void JsonFile::dumpToTerminal()
{
	auto output = json.dump(1, '\t');
	std::cout.write(output.data(), output.size());
	std::cout.put('\n');
	std::cout.flush();
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
void JsonFile::makeNode(const char* inKey, const JsonDataType inType)
{
	if (json.contains(inKey))
	{
		if (json.at(inKey).type() == inType)
			return;
	}

	json[inKey] = initializeDataType(inType);
	setDirty(true);
}

/*****************************************************************************/
bool JsonFile::assignStringIfEmptyWithFallback(Json& outNode, const char* inKey, const std::string& inValueA, const std::string& inValueB, const std::function<void()>& onAssignA)
{
	if (!outNode.contains(inKey) || !outNode.at(inKey).is_string())
	{
		outNode[inKey] = inValueB;
		setDirty(true);
	}

	auto value = outNode.at(inKey).get<std::string>();
	if (!inValueA.empty() && inValueA != value)
	{
		if (onAssignA != nullptr)
			onAssignA();

		outNode[inKey] = inValueA;
		setDirty(true);
	}

	return true;
}

/*****************************************************************************/
std::string JsonFile::getSchema()
{
	std::string ret;

	// don't think this even worked...
	if (json.contains("$schema"))
	{
		if (json.at("$schema").is_string())
		{
			ret = json.get<std::string>();
			// json.erase("$schema");
		}
	}

	return ret;
}

/*****************************************************************************/
bool JsonFile::validate(Json&& inSchemaJson)
{
	if (m_filename.empty())
		return false;

	JsonValidator validator(m_filename);
	if (!validator.setSchema(std::move(inSchemaJson)))
		return false;

	// false if any errors
	if (!validator.validate(json))
	{
		// false if fatal error
		if (!validator.printErrors())
		{
			Diagnostic::error("Failed to validate the json file: {}", m_filename);
			return false;
		}
	}

	return true;
}

/*****************************************************************************/
Json JsonFile::initializeDataType(const JsonDataType inType)
{
	switch (inType)
	{
		case JsonDataType::object:
			return Json::object();

		case JsonDataType::array:
			return Json::array();

		case JsonDataType::string:
			return std::string();

		case JsonDataType::binary:
			return 0x0;

		case JsonDataType::boolean:
			return false;

		case JsonDataType::number_float:
			return 0.0f;

		case JsonDataType::number_integer:
			return 0;

		case JsonDataType::number_unsigned:
			return 0U;

		default:
			break;
	}

	return Json();
}

}
