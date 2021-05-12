/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_PATH_HPP
#define CHALET_PATH_HPP

namespace chalet
{
namespace Path
{
void sanitize(std::string& outValue, const bool inRemoveNewLine = false);
void sanitizeForWindows(std::string& outValue, const bool inRemoveNewLine = false);
void sanitizeWithDrives(std::string& outPath);
void windowsDrivesToMsysDrives(std::string& outPath);
void msysDrivesToWindowsDrives(std::string& outPath);
StringList getOSPaths();
constexpr char getSeparator();
}
}

#include "Terminal/Path.inl"

#endif // CHALET_PATHS_HPP
