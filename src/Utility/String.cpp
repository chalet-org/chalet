/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Utility/String.hpp"

#include "Terminal/Environment.hpp"
#include "Utility/List.hpp"

namespace chalet
{
/*****************************************************************************/
bool String::equals(const std::string_view inCompare, const std::string& inString)
{
	return ::strcmp(inString.c_str(), inCompare.data()) == 0;
}

/*****************************************************************************/
bool String::equals(const char inCompare, const std::string& inString) noexcept
{
	return inString.size() == 1 && inString.front() == inCompare;
}

/*****************************************************************************/
bool String::equals(const StringList& inFind, const std::string& inString)
{
	for (auto& item : inFind)
	{
		if (equals(item, inString))
			return true;
	}
	return false;
}

/*****************************************************************************/
bool String::contains(const std::string_view inFind, const std::string& inString)
{
	auto result = inString.find(inFind);
	return result != std::string::npos;
}

/*****************************************************************************/
bool String::contains(const char inFind, const std::string& inString) noexcept
{
	return inString.find(inFind) != std::string::npos;
}

/*****************************************************************************/
bool String::contains(const StringList& inFind, const std::string& inString)
{
	if (inString.empty())
		return false;

	for (auto& item : inFind)
	{
		if (contains(item, inString))
			return true;
	}
	return false;
}

/*****************************************************************************/
bool String::startsWith(const std::string_view inStart, const std::string& inString)
{
	if (inString.size() < inStart.size())
		return false;

	return std::equal(inStart.begin(), inStart.end(), inString.begin());
}

/*****************************************************************************/
bool String::startsWith(const char inStart, const std::string& inString) noexcept
{
	if (inString.empty())
		return false;

	return inString.front() == inStart;
}

/*****************************************************************************/
bool String::startsWith(const StringList& inFind, const std::string& inString)
{
	if (inString.empty())
		return false;

	for (auto& item : inFind)
	{
		if (startsWith(item, inString))
			return true;
	}
	return false;
}

/*****************************************************************************/
bool String::endsWith(const std::string_view inEnd, const std::string& inString)
{
	if (inEnd.size() > inString.size())
		return false;

	return std::equal(inEnd.rbegin(), inEnd.rend(), inString.rbegin());
}

/*****************************************************************************/
bool String::endsWith(const char inEnd, const std::string& inString) noexcept
{
	if (inString.empty())
		return false;

	return inString.back() == inEnd;
}

/*****************************************************************************/
bool String::endsWith(const StringList& inFind, const std::string& inString)
{
	if (inString.empty())
		return false;

	for (auto& item : inFind)
	{
		if (endsWith(item, inString))
			return true;
	}
	return false;
}

/*****************************************************************************/
bool String::onlyContainsCharacters(const std::string& inChars, const std::string& inString)
{
	return inString.find_first_not_of(inChars) != std::string::npos;
}

/*****************************************************************************/
std::string String::fromBoolean(const bool inValue) noexcept
{
	return inValue ? "On" : "Off";
}

/*****************************************************************************/
void String::replaceAll(std::string& outString, const std::string_view inFrom, const std::string_view inTo)
{
	if (inFrom.empty())
		return;

	std::size_t pos = 0;
	while ((pos = outString.find(inFrom, pos)) != std::string::npos)
	{
		outString.replace(pos, inFrom.length(), inTo);
		pos += inTo.length();
	}

	return;
}

/*****************************************************************************/
void String::replaceAll(std::string& outString, const char inFrom, const char inTo)
{
	return std::replace(outString.begin(), outString.end(), inFrom, inTo);
}

/*****************************************************************************/
std::string String::toLowerCase(const std::string& inString)
{
	std::string ret = inString;
	std::transform(ret.begin(), ret.end(), ret.begin(), [](std::string::value_type c) {
		return static_cast<std::string::value_type>(::tolower(c));
	});
	return ret;
}

/*****************************************************************************/
std::string String::toUpperCase(const std::string& inString)
{
	std::string ret = inString;
	std::transform(ret.begin(), ret.end(), ret.begin(), [](std::string::value_type c) {
		return static_cast<std::string::value_type>(::toupper(c));
	});
	return ret;
}

/*****************************************************************************/
std::string String::join(const StringList& inList, const char inSeparator)
{
	std::string ret;

	for (auto& item : inList)
	{
		if (item.empty())
			continue;

		std::ptrdiff_t i = &item - &inList.front();
		if (i > 0)
			ret += inSeparator;

		ret += item;
	}

	return ret;
}

/*****************************************************************************/
std::string String::join(StringList&& inList, const char inSeparator)
{
	std::string ret;

	for (auto& item : inList)
	{
		if (item.empty())
			continue;

		std::ptrdiff_t i = &item - &inList.front();
		if (i > 0)
			ret += inSeparator;

		ret += std::move(item);
	}

	return ret;
}

/*****************************************************************************/
std::string String::join(const StringList& inList, const std::string_view inSeparator)
{
	std::string ret;

	for (auto& item : inList)
	{
		if (item.empty())
			continue;

		std::ptrdiff_t i = &item - &inList.front();
		if (i > 0)
			ret += inSeparator;

		ret += item;
	}

	return ret;
}

/*****************************************************************************/
std::string String::join(StringList&& inList, const std::string_view inSeparator)
{
	std::string ret;

	for (auto& item : inList)
	{
		if (item.empty())
			continue;

		std::ptrdiff_t i = &item - &inList.front();
		if (i > 0)
			ret += inSeparator;

		ret += std::move(item);
	}

	return ret;
}

/*****************************************************************************/
StringList String::split(std::string inString, const char inSeparator, const std::size_t inMinLength)
{
	StringList ret;

	std::size_t itr = 0;
	while (itr != std::string::npos)
	{
		itr = inString.find(inSeparator);

		std::string sub = inString.substr(0, itr);
		std::size_t nextNonChar = inString.find_first_not_of(inSeparator, itr);

		const bool nonCharFound = nextNonChar != std::string::npos;
		inString = inString.substr(nonCharFound ? nextNonChar : itr + 1);
		if (nonCharFound)
			itr = nextNonChar;

		if (!sub.empty())
		{
			while (sub.back() == inSeparator)
				sub.pop_back();
		}

		if (sub.size() >= inMinLength)
			ret.push_back(sub);
	}

	return ret;
}

/*****************************************************************************/
StringList String::split(std::string inString, const std::string_view inSeparator, const std::size_t inMinLength)
{
	StringList ret;

	std::size_t itr = 0;
	while (itr != std::string::npos)
	{
		itr = inString.find(inSeparator);

		std::string sub = inString.substr(0, itr);
		std::size_t nextNonChar = inString.find_first_not_of(inSeparator, itr);

		const bool nonCharFound = nextNonChar != std::string::npos;
		inString = inString.substr(nonCharFound ? nextNonChar : itr + 1);
		if (nonCharFound)
			itr = nextNonChar;

		if (!sub.empty())
		{
			while (inSeparator.size() == 1 && sub.back() == inSeparator.front())
				sub.pop_back();
		}

		if (sub.size() >= inMinLength)
			ret.push_back(sub);
	}

	return ret;
}

/*****************************************************************************/
std::string String::getPrefixed(const StringList& inList, const std::string& inPrefix)
{
	if (inList.empty())
		return std::string();

	std::string ret = String::join(inList, " " + inPrefix);
	if (!ret.empty())
		ret = inPrefix + ret;

	if (ret.front() == ' ')
		return ret.substr(1);

	return ret;
}

/*****************************************************************************/
std::string String::getSuffixed(const StringList& inList, const std::string& inSuffix)
{
	std::string ret = String::join(inList, inSuffix + ' ');

	if (inList.size() > 0)
		ret += inSuffix;

	return ret;
}

/*****************************************************************************/
std::string String::getPrefixedAndSuffixed(const StringList& inList, const std::string& inPrefix, const std::string& inSuffix)
{
	if (inList.empty())
		return std::string();

	std::string ret = String::join(inList, inSuffix + " " + inPrefix);
	if (!ret.empty())
	{
		ret = inPrefix + ret;
		ret += inSuffix;
	}

	if (ret.front() == ' ')
		return ret.substr(1);

	return ret;
}

/*****************************************************************************/
StringList String::filterIf(const StringList& inFind, const StringList& inList)
{
	StringList ret;
	for (auto& item : inFind)
	{
		if (item.empty())
			continue;

		if (List::contains(inList, item))
			ret.push_back(item);
	}
	return ret;
}

/*****************************************************************************/
StringList String::excludeIf(const StringList& inFind, const StringList& inList)
{
	StringList ret;
	for (auto& item : inList)
	{
		if (item.empty())
			continue;

		if (!List::contains(inFind, item))
			ret.push_back(item);
	}
	return ret;
}

/*****************************************************************************/
std::string String::getPathSuffix(const std::string& inPath)
{
	return inPath.substr(inPath.find_last_of('.') + 1);
}

/*****************************************************************************/
std::string String::getPathBaseName(const std::string& inPath)
{
	if (inPath.empty())
		return inPath;

	const auto& pathNoExt = inPath.substr(0, inPath.find_last_of('.'));
	return pathNoExt.substr(pathNoExt.find_last_of('/') + 1);
}

/*****************************************************************************/
std::string String::getPathFolder(const std::string& inPath)
{
	auto end = inPath.find_last_of('/');
	return end != std::string::npos ? inPath.substr(0, end) : "";
}

/*****************************************************************************/
std::string String::getRootFolder(const std::string& inPath)
{
	auto end = inPath.find_first_of('/');
	return end != std::string::npos ? inPath.substr(0, end) : "";
}

/*****************************************************************************/
std::string String::getPathFilename(const std::string& inPath)
{
	return inPath.substr(inPath.find_last_of('/') + 1);
}

/*****************************************************************************/
std::string String::getPathFolderBaseName(const std::string& inPath)
{
	auto end = inPath.find_last_of('.');
	return end != std::string::npos ? inPath.substr(0, end) : inPath;
}

/*****************************************************************************/
bool String::isWrapped(const std::string& inString, const std::string_view inStart, const std::string_view inEnd)
{
	if (!String::startsWith(inStart, inString))
		return false;

	if (!String::endsWith(inEnd, inString))
		return false;

	return true;
}

/*****************************************************************************/
std::string String::eol()
{
#if defined(CHALET_WIN32)
	return "\r\n";
#else
	return "\n";
#endif
}
}