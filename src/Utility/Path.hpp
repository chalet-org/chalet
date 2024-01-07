/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

namespace chalet
{
namespace Path
{
void toUnix(std::string& outValue, const bool inRemoveNewLine = false);
void toWindows(std::string& outValue, const bool inRemoveNewLine = false);
void capitalizeDriveLetter(std::string& outValue);
std::string getWithSeparatorSuffix(const std::string& inPath);
}
}
