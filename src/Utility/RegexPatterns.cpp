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
	if (inValue.empty())
		return false;

#ifdef CHALET_REGEX_CTRE
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
	if (inValue.empty())
		return false;

#ifdef CHALET_REGEX_CTRE
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
bool RegexPatterns::matchesCxxStandardShort(const std::string& inValue)
{
	if (inValue.empty())
		return false;

#ifdef CHALET_REGEX_CTRE
	static constexpr auto regex = ctll::fixed_string{ "^\\d[\\dxyzab]$" };
	if (auto m = ctre::match<regex>(inValue))
#else
	static std::regex regex{ "^\\d[\\dxyzab]$" };
	if (std::regex_match(inValue, regex))
#endif
	{
		return true;
	}

	return false;
}

/*****************************************************************************/
std::string RegexPatterns::matchesTargetArchitectureWithResult(const std::string& inValue)
{
	std::string result;

	// #ifdef CHALET_REGEX_CTRE
	// 	static constexpr auto regex = ctll::fixed_string{ "^.*?((x64|x86)?_?(x86|x64|arm64|arm)).*?$" };
	// 	if (auto m = ctre::match<regex>(inValue))
	// 	{
	// 		result = m.get<1>();
	// 	}
	// #else
	static std::regex regex{ "^.*?((x64|x86)?_?(x86|x64|arm64|arm)).*?$" };
	std::smatch m;
	if (std::regex_match(inValue, m, regex))
	{
		if (m.size() >= 2)
		{
			result = m[1].str();
		}
	}
	// #endif

	return result;
}

/*****************************************************************************/
bool RegexPatterns::matchesFullVersionString(const std::string& inValue)
{
	if (inValue.empty())
		return false;

		// This pattern doesn't match the number of digits because it's not worth creating a bug over
		//

#ifdef CHALET_REGEX_CTRE
	static constexpr auto regex = ctll::fixed_string{ "^(\\d+\\.\\d+\\.\\d+\\.\\d+)$" };
	if (auto m = ctre::match<regex>(inValue))
#else
	static std::regex regex{ "^(\\d+\\.\\d+\\.\\d+\\.\\d+)$" };
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
	std::regex pattern(inValue);
	outText = std::regex_replace(outText, pattern, inReplaceValue);

	return true;
}
}
