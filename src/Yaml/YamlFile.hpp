/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include "Libraries/Json.hpp"

namespace chalet
{
struct YamlFile
{
	static bool parse(Json& outJson, const std::string& inFilename, const bool inError = true);
	static bool parseLiteral(Json& outJson, const std::string& inContents, const bool inError = true);
	static bool saveToFile(const Json& inJson, const std::string& inFilename);
	static std::string asString(const Json& inJson);

private:
	YamlFile() = delete;
	explicit YamlFile(const std::string& inFilename);

	bool parseAsJson(Json& outJson) const;
	bool parseAsJson(Json& outJson, const std::string& inContents) const;
	bool parseAsJson(Json& outJson, std::istream& stream) const;
	bool save(const Json& inJson);

	std::string getNodeAsString(const std::string& inKey, const Json& inValue, const size_t inIndent = 0, const bool inArray = false) const;

	StringList parseAbbreviatedStringList(const std::string& inValue) const;
	std::vector<f32> parseAbbreviatedNumericList(std::string inString) const;
	Json parseAbbreviatedObject(const std::string& inValue) const;

	const std::string& m_filename;
};
}
