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
	static bool generate(const Json& inJson, const std::string& inFilename);

private:
	YamlFile() = delete;
	explicit YamlFile(const std::string& inFilename);

	bool parseAsJson(Json& outJson);
	bool save(const Json& inJson);

	std::string getNodeAsString(const std::string& inKey, const Json& inValue, const size_t inIndent = 0, const bool inArray = false) const;

	const std::string& m_filename;
};
}
