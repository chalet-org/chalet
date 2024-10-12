/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Json/JsonFile.hpp"

#include "System/Files.hpp"
#include "Terminal/Output.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"
#include "Yaml/YamlFile.hpp"
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
bool JsonFile::saveToFile(const Json& inJson, const std::string& outFilename, const i32 inIndent)
{
	if (outFilename.empty())
		return false;

	const auto folder = String::getPathFolder(outFilename);
	if (!folder.empty() && !Files::pathExists(folder))
	{
		if (!Files::makeDirectory(folder))
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
	if (String::endsWith(".yaml", m_filename))
	{
		return YamlFile::parse(root, m_filename, inError);
	}
	else
	{
		return JsonComments::parse(root, m_filename, inError);
	}
}

/*****************************************************************************/
bool JsonFile::load(std::string inFilename, const bool inError)
{
	m_filename = std::move(inFilename);

	return load(inError);
}

/*****************************************************************************/
bool JsonFile::save(const i32 inIndent)
{
	if (!m_filename.empty() && m_dirty)
	{
		bool result = JsonFile::saveToFile(root, m_filename, inIndent);
		m_dirty = false;
		return result;
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
	root = Json::object();
	setDirty(true);
	save();
}

/*****************************************************************************/
void JsonFile::dumpToTerminal()
{
	auto output = root.dump(1, '\t');
	std::cout.write(output.data(), output.size());
	std::cout.put('\n');
	std::cout.flush();
}

/*****************************************************************************/
bool JsonFile::saveAs(const std::string& inFilename, const i32 inIndent) const
{
	if (String::endsWith(".yaml", inFilename))
	{
		return YamlFile::saveToFile(root, inFilename);
	}
	else
	{
		return JsonFile::saveToFile(root, inFilename, inIndent);
	}
}

/*****************************************************************************/
void JsonFile::setContents(Json&& inJson)
{
	root = std::move(inJson);
}

/*****************************************************************************/
const std::string& JsonFile::filename() const noexcept
{
	return m_filename;
}

/*****************************************************************************/
void JsonFile::makeNode(const char* inKey, const JsonDataType inType)
{
	if (root.contains(inKey))
	{
		if (root.at(inKey).type() == inType)
			return;
	}

	root[inKey] = initializeDataType(inType);
	setDirty(true);
}

/*****************************************************************************/
bool JsonFile::assignNodeIfEmptyWithFallback(Json& outNode, const char* inKey, const std::string& inValueA, const std::string& inValueB)
{
	if (!outNode.contains(inKey) || !outNode.at(inKey).is_string())
	{
		outNode[inKey] = inValueB;
		setDirty(true);
	}

	auto value = outNode.at(inKey).get<std::string>();
	if (!inValueA.empty() && inValueA != value)
	{
		outNode[inKey] = inValueA;
		setDirty(true);
	}

	return true;
}

/*****************************************************************************/
bool JsonFile::assignNodeWithFallback(Json& outNode, const char* inKey, const std::string& inValueA, const std::string& inValueB)
{
	if (!outNode.contains(inKey) || !outNode.at(inKey).is_string())
	{
		outNode[inKey] = inValueB;
		setDirty(true);
	}

	auto value = outNode.at(inKey).get<std::string>();
	if (!inValueA.empty() && !value.empty() && inValueA != value)
	{
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
	if (root.contains("$schema"))
	{
		if (root.at("$schema").is_string())
		{
			ret = root.get<std::string>();
			// data.erase("$schema");
		}
	}

	return ret;
}

/*****************************************************************************/
bool JsonFile::validate(const Json& inSchemaJson)
{
	if (m_filename.empty())
		return false;

	JsonValidator validator;
	if (!validator.setSchema(inSchemaJson))
		return false;

	JsonValidationErrors errors;
	// false if any errors
	if (!validator.validate(root, m_filename, errors))
	{
		// false if fatal error
		if (!validator.printErrors(errors))
		{
			Diagnostic::error("Failed to validate the file: {}", m_filename);
		}
		else
		{
			// There was an exception
			Diagnostic::error("An internal error occurred getting the validation details for: {}", m_filename);
		}
		return false;
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
