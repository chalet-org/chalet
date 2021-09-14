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

	bool load();
	bool load(std::string inFilename);
	bool save(const int inIndent = 1);

	bool dirty() const noexcept;
	void setDirty(const bool inValue) noexcept;

	void dumpToTerminal();

	void setContents(Json&& inJson);

	const std::string& filename() const noexcept;
	void makeNode(const std::string& inName, const JsonDataType inType);

	bool validate(Json&& inSchemaJson);

	template <typename T>
	bool assignFromKey(T& outVariable, const Json& inNode, const std::string& inKey);

	template <typename T>
	bool assignNodeIfEmpty(Json& outNode, const std::string& inKey, const std::function<T()>& onAssign);

	bool assignStringAndValidate(std::string& outString, const Json& inNode, const std::string& inKey, const std::string& inDefault = "");
	bool assignStringListAndValidate(StringList& outList, const Json& inNode, const std::string& inKey);

	bool assignStringIfEmptyWithFallback(Json& outNode, const std::string& inKey, const std::string& inValueA, const std::string& inValueB, const std::function<void()>& onAssignA = nullptr);

	template <typename T>
	bool containsKeyForType(const Json& inNode, const std::string& inKey);

	bool containsKeyThatStartsWith(const Json& inNode, const std::string& inFind);

	Json json;

private:
	void initializeDataType(Json& inJson, const JsonDataType inType);

	void warnBlankKey(const std::string& inKey, const std::string& inDefault = "");
	void warnBlankKeyInList(const std::string& inKey);

	std::string m_filename;

	bool m_dirty = false;
};
}

#include "Json/JsonFile.inl"

#endif // CHALET_JSON_FILE_HPP
