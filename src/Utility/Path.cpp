/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Utility/Path.hpp"

#include "Process/Environment.hpp"
#include "System/Files.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
void Path::toUnix(std::string& outValue, const bool inRemoveNewLine)
{
	if (outValue.empty())
		return;

	if (inRemoveNewLine)
	{
		String::replaceAll(outValue, '\r', ' ');
		String::replaceAll(outValue, '\n', ' ');
	}

	String::replaceAll(outValue, "\\\\", "/");
	String::replaceAll(outValue, '\\', '/');

	if (String::endsWith("/.", outValue))
	{
		outValue.pop_back();
		outValue.pop_back();
	}
	else
	{
		while (outValue.back() == ' ')
			outValue.pop_back();

		while (outValue.back() == '/')
			outValue.pop_back();
	}

#if defined(CHALET_WIN32)
	Path::capitalizeDriveLetter(outValue);
#endif
}

/*****************************************************************************/
void Path::toWindows(std::string& outValue, const bool inRemoveNewLine)
{
#if defined(CHALET_WIN32)
	if (outValue.empty())
		return;

	if (inRemoveNewLine)
	{
		String::replaceAll(outValue, '\r', ' ');
		String::replaceAll(outValue, '\n', ' ');
	}

	String::replaceAll(outValue, "\\\\", "\\");
	String::replaceAll(outValue, '/', '\\');

	if (outValue.back() == ' ')
		outValue.pop_back();

	Path::capitalizeDriveLetter(outValue);
#else
	Path::toUnix(outValue, inRemoveNewLine);
#endif
}

/*****************************************************************************/
void Path::capitalizeDriveLetter(std::string& outValue)
{
	if (outValue.size() >= 3 && outValue[1] == ':' && outValue[2] == '/')
	{
		String::capitalize(outValue);
	}
}

/*****************************************************************************/
std::string Path::getWithSeparatorSuffix(const std::string& inPath)
{
	auto path = inPath;
	Path::toUnix(path);
	if (path.back() != '/')
		path += '/';

	return path;
}
}
