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
	initializeDataType(json[inName], inType);
}

/*****************************************************************************/
bool JsonFile::assignStringAndValidate(std::string& outString, const Json& inNode, const std::string& inKey, const std::string& inDefault)
{
	bool result = assignFromKey(outString, inNode, inKey);
	if (result && outString.empty())
	{
		warnBlankKey(inKey, inDefault);
		return false;
	}

	return result;
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
		if (item.empty())
			warnBlankKeyInList(inKey);

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
bool JsonFile::containsKeyThatStartsWith(const Json& inNode, const std::string& inFind)
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
			return;
	}

	setDirty(true);
}

/*****************************************************************************/
void JsonFile::warnBlankKey(const std::string& inKey, const std::string& inDefault)
{
	if (!inDefault.empty())
		Diagnostic::warn("{}: '{}' was defined, but blank. Using the built-in default ({})", m_filename, inKey, inDefault);
	else
		Diagnostic::warn("{}: '{}' was defined, but blank.", m_filename, inKey);
}

/*****************************************************************************/
void JsonFile::warnBlankKeyInList(const std::string& inKey)
{
	Diagnostic::warn("{}: A blank value was found in '{}'.", m_filename, inKey);
}

}
