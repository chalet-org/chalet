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
		Files::ofstream(outFilename) << json::dump(inJson, 1, '\t') << std::endl;
	else
		Files::ofstream(outFilename) << json::dump(inJson, inIndent) << std::endl;

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
	auto output = json::dump(root, 1, '\t');
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
std::string JsonFile::getSchema() const
{
	std::string ret;

	// don't think this even worked...
	const char* kSchemaId = "$schema";
	if (root.contains(kSchemaId))
	{
		if (root[kSchemaId].is_string())
		{
			ret = root.get<std::string>();
			// data.erase(kSchemaId);
		}
	}

	return ret;
}

/*****************************************************************************/
bool JsonFile::validate(const Json& inSchemaJson) const
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
}
