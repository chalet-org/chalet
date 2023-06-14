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

	static bool saveToFile(const Json& inJson, const std::string& outFilename, const int inIndent = 1);

	bool load(const bool inError = true);
	bool load(std::string inFilename, const bool inError = true);
	bool save(const int inIndent = 1);

	bool dirty() const noexcept;
	void setDirty(const bool inValue) noexcept;

	void resetAndSave();

	void dumpToTerminal();

	void setContents(Json&& inJson);

	const std::string& filename() const noexcept;
	void makeNode(const char* inKey, const JsonDataType inType);

	std::string getSchema();
	bool validate(Json&& inSchemaJson);

	template <typename T>
	bool assignFromKey(T& outVariable, const Json& inNode, const char* inKey) const;

	template <typename T>
	bool assignNodeIfEmpty(Json& outNode, const char* inKey, const T& inValue);

	template <typename T>
	bool assignNodeIfEmptyWithFallback(Json& outNode, const char* inKey, const std::optional<T>& inValueA, const T& inValueB);

	bool assignNodeIfEmptyWithFallback(Json& outNode, const char* inKey, const std::string& inValueA, const std::string& inValueB);
	bool assignNodeWithFallback(Json& outNode, const char* inKey, const std::string& inValueA, const std::string& inValueB);

	template <typename T>
	bool containsKeyForType(const Json& inNode, const char* inKey) const;

	Json json;

private:
	Json initializeDataType(const JsonDataType inType);

	std::string m_filename;

	bool m_dirty = false;
};
}

#include "Json/JsonFile.inl"

#endif // CHALET_JSON_FILE_HPP
