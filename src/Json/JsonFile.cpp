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

	if (inIndent <= 0 || inIndent > 4)
		std::ofstream(outFilename) << inJson.dump() << std::endl;
	else
		std::ofstream(outFilename) << inJson.dump(inIndent, '\t') << std::endl;

	return true;
}

/*****************************************************************************/
bool JsonFile::load()
{
	chalet_assert(!m_filename.empty(), "JsonFile::load(): No file to load");
	return JsonComments::parse(json, m_filename);
}

/*****************************************************************************/
bool JsonFile::load(std::string inFilename)
{
	m_filename = std::move(inFilename);

	return load();
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
void JsonFile::dumpToTerminal()
{
	std::cout << json.dump(1, '\t') << std::endl;
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
	if (json.contains(inName))
	{
		if (json.at(inName).type() == inType)
			return;
	}

	json[inName] = initializeDataType(inType);
	setDirty(true);
}

/*****************************************************************************/
bool JsonFile::assignStringListAndValidate(StringList& outList, const Json& inNode, const std::string& inKey)
{
	if (!inNode.contains(inKey))
		return false;

	const Json& list = inNode.at(inKey);
	if (!list.is_array())
		return false;

	for (auto& itemRaw : list)
	{
		if (!itemRaw.is_string())
			return false;

		std::string item = itemRaw.get<std::string>();
		List::addIfDoesNotExist(outList, std::move(item));
	}

	return true;
}

/*****************************************************************************/
bool JsonFile::assignStringIfEmptyWithFallback(Json& outNode, const std::string& inKey, const std::string& inValueA, const std::string& inValueB, const std::function<void()>& onAssignA)
{
	bool result = false;
	bool validString = outNode.contains(inKey) && outNode.at(inKey).is_string();

	std::string value;
	if (validString)
	{
		value = outNode.at(inKey).get<std::string>();
		validString &= !value.empty() && inValueA.empty();
	}

	if (!validString)
	{
		if (!inValueA.empty())
		{
			if (onAssignA != nullptr)
				onAssignA();

			outNode[inKey] = inValueA;
		}
		else
		{
			outNode[inKey] = inValueB;
		}
		setDirty(true);
	}

	return result;
}

/*****************************************************************************/
bool JsonFile::containsKeyThatStartsWith(const Json& inNode, const std::string& inFind) const
{
	bool res = false;

	for (auto& [key, value] : inNode.items())
	{
		res |= String::startsWith(inFind, key);
	}

	return res;
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
			Diagnostic::error("{}: Failed to validate against its schema.", m_filename);
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
