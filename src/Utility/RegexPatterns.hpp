/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_REGEX_PATTERNS_HPP
#define CHALET_REGEX_PATTERNS_HPP

namespace chalet
{
namespace RegexPatterns
{
bool matchesGnuCppStandard(const std::string& inValue);
bool matchesGnuCStandard(const std::string& inValue);
}
}

#endif // CHALET_REGEX_PATTERNS_HPP
