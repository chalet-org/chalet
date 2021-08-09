/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Utility/RegexPatterns.hpp"

#include "Libraries/Regex.hpp"

namespace chalet
{
/*****************************************************************************/
bool RegexPatterns::matchesGnuCppStandard(const std::string& inValue)
{
#if !defined(CHALET_MSVC) && !(defined(CHALET_CLANG) && defined(CHALET_LINUX))
	static constexpr auto regex = ctll::fixed_string{ "^(c|gnu)\\+\\+\\d[\\dxyzab]$" };
	if (auto m = ctre::match<regex>(inValue))
#else
	static std::regex regex{ "^(c|gnu)\\+\\+\\d[\\dxyzab]$" };
	if (std::regex_match(inValue, regex))
#endif
	{
		return true;
	}

	return false;
}

/*****************************************************************************/
bool RegexPatterns::matchesGnuCStandard(const std::string& inValue)
{
#if !defined(CHALET_MSVC) && !(defined(CHALET_CLANG) && defined(CHALET_LINUX))
	static constexpr auto regex = ctll::fixed_string{ "^((c|gnu)\\d[\\dx]|(iso9899:(1990|199409|1999|199x|20\\d{2})))$" };
	if (auto m = ctre::match<regex>(inValue))
#else
	static std::regex regex{ "^((c|gnu)\\d[\\dx]|(iso9899:(1990|199409|1999|199x|20\\d{2})))$" };
	if (std::regex_match(inValue, regex))
#endif
	{
		return true;
	}

	return false;
}

/*****************************************************************************/
bool RegexPatterns::matchAndReplace(std::string& outText, const std::string& inValue, const std::string& inReplaceValue)
{
	// replace value needs $& syntax
	std::regex vowel_re(inValue);
	outText = std::regex_replace(outText, vowel_re, inReplaceValue);

	return true;
}
}
