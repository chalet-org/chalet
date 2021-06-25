/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Terminal/Path.hpp"

#include "Terminal/Commands.hpp"
#include "Terminal/Environment.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
void Path::sanitize(std::string& outValue, const bool inRemoveNewLine)
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

	if (outValue.back() == ' ')
		outValue.pop_back();
}

/*****************************************************************************/
void Path::sanitizeForWindows(std::string& outValue, const bool inRemoveNewLine)
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
#else
	sanitize(outValue, inRemoveNewLine);
#endif
}

/*****************************************************************************/
StringList Path::getOSPaths()
{
#if !defined(CHALET_WIN32)
	return {
		"/usr/local/sbin",
		"/usr/local/bin",
		"/usr/sbin",
		"/usr/bin",
		"/sbin",
		"/bin"
	};
#endif

	return {};
}
}
