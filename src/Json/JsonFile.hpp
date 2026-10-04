/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include "Libraries/Json.hpp"

namespace chalet
{
struct JsonFile
{
	JsonFile() = default;
	explicit JsonFile(std::string inFilename);

	static bool saveToFile(const Json& inJson, const std::string& outFilename, const i32 inIndent = 1);

	bool load(const bool inError = true);
	bool load(std::string inFilename, const bool inError = true);
	bool save(const i32 inIndent = 1);

	bool dirty() const noexcept;
	void setDirty(const bool inValue) noexcept;

	void resetAndSave();

	void dumpToTerminal();

	bool saveAs(const std::string& inFilename, const i32 inIndent = 1) const;

	void setContents(Json&& inJson);

	const std::string& filename() const noexcept;

	std::string getSchema() const;
	bool validate(const Json& inSchemaJson) const;

	Json root;

private:
	std::string m_filename;

	bool m_dirty = false;
};
}
