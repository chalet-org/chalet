/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

namespace chalet
{
namespace RegexPatterns
{
bool matchesGnuCppStandard(const std::string& inValue);
bool matchesGnuCStandard(const std::string& inValue);
bool matchesCxxStandardShort(const std::string& inValue);
bool matchesFullVersionString(const std::string& inValue);
bool matchAndReplace(std::string& outText, const std::string& inValue, const std::string& inReplaceValue);
bool matchAndReplaceConfigureFileVariables(std::string& outText, const std::function<std::string(std::string)>& onMatch);
bool matchAndReplacePathVariables(std::string& outText, const std::function<std::string(std::string, bool&)>& onMatch);
}
}
