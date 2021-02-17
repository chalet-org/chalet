/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_STRING_HPP
#define CHALET_STRING_HPP

namespace chalet
{
namespace String
{
bool contains(const std::string_view& inFind, const std::string& inString);
bool startsWith(const std::string_view& inFind, const std::string& inString);
bool endsWith(const std::string_view& inFind, const std::string& inString);
bool onlyContainsCharacters(const std::string& inChars, const std::string& inString);
std::string fromBoolean(const bool inValue) noexcept;
void replaceAll(std::string& outString, const std::string_view& inFrom, const std::string_view& inTo);
void replaceAll(std::string& outString, const char inFrom, const char inTo);
std::string toLowerCase(std::string inString);
std::string toUpperCase(std::string inString);
std::string join(const StringList& inList, const std::string_view& inSeparator = " ");
StringList split(std::string inString, const std::string_view& inSeparator = " ");
std::string getPrefixed(const StringList& inList, const std::string& inPrefix);
std::string getSuffixed(const StringList& inList, const std::string& inSuffix);
std::string getPrefixedAndSuffixed(const StringList& inList, const std::string& inPrefix, const std::string& inSuffix);
StringList filterIf(const std::vector<std::string>& inFind, const StringList& inList);
std::string getPathSuffix(const std::string& inPath);
std::string getPathBaseName(const std::string& inPath);
std::string getPathFolder(const std::string& inPath);
std::string getPathFilename(const std::string& inPath);
std::string getPathFolderBaseName(const std::string& inPath);
bool isWrapped(const std::string& inString, const std::string_view& inStart, const std::string_view& inEnd);

inline bool equals(const std::string& inString, const char* inCompare);
}
}

#include "Utility/String.inl"

#endif // CHALET_STRING_HPP
