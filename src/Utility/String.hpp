/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

namespace chalet
{
namespace String
{
bool equals(const std::string_view inCompare, const std::string& inString);
bool equals(const char inCompare, const std::string& inString) noexcept;
bool equals(const StringList& inFind, const std::string& inString);

bool contains(const std::string_view inFind, const std::string& inString);
bool contains(const char inFind, const std::string& inString) noexcept;
bool contains(const StringList& inFind, const std::string& inString);

bool startsWith(const std::string_view inFind, const std::string& inString);
bool startsWith(const char inFind, const std::string& inString) noexcept;
bool startsWith(const StringList& inFind, const std::string& inString);

bool endsWith(const std::string_view inFind, const std::string& inString);
bool endsWith(const char inFind, const std::string& inString) noexcept;
bool endsWith(const StringList& inFind, const std::string& inString);

bool capitalize(std::string& outString);
bool decapitalize(std::string& outString);

bool onlyContainsCharacters(const std::string& inChars, const std::string& inString);
std::string fromBoolean(const bool inValue) noexcept;
void replaceAll(std::string& outString, const std::string_view inFrom, const std::string_view inTo);
void replaceAll(std::string& outString, const char inFrom, const std::string_view inTo);
void replaceAll(std::string& outString, const char inFrom, const char inTo);
std::string toLowerCase(const std::string& inString);
std::string toUpperCase(const std::string& inString);
std::string join(const StringList& inList, const char inSeparator = ' ');
std::string join(StringList&& inList, const char inSeparator = ' ');
std::string join(const StringList& inList, const std::string_view inSeparator);
std::string join(StringList&& inList, const std::string_view inSeparator);
StringList split(std::string inString, const char inSeparator = ' ', const size_t inMinLength = 0);
StringList split(std::string inString, const std::string_view inSeparator, const size_t inMinLength = 0);
std::string getPrefixed(const StringList& inList, const std::string& inPrefix);
std::string getSuffixed(const StringList& inList, const std::string& inSuffix);
std::string getPrefixedAndSuffixed(const StringList& inList, const std::string& inPrefix, const std::string& inSuffix);
StringList filterIf(const StringList& inFind, const StringList& inList);
StringList excludeIf(const StringList& inFind, const StringList& inList);
std::string getPathSuffix(const std::string& inPath);
std::string getPathBaseName(const std::string& inPath);
std::string getPathFolder(const std::string& inPath);
std::string getRootFolder(const std::string& inPath);
std::string getPathFilename(const std::string& inPath);
std::string getPathFolderBaseName(const std::string& inPath);
bool isWrapped(const std::string& inString, const std::string_view inStart, const std::string_view inEnd);
std::string withByteOrderMark(const std::string& inString);
std::string convertUnicodeToHex(const std::string& inString);

#if defined(CHALET_WIN32)
std::wstring toWideString(const std::string& inValue, u32 codePage = 0);
std::string fromWideString(const std::wstring& inValue, u32 codePage = 0);
#endif

std::string eol();

}
}
