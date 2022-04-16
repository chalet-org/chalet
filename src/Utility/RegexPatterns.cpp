/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Utility/RegexPatterns.hpp"

#include "Libraries/Regex.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
bool RegexPatterns::matchesGnuCppStandard(const std::string& inValue)
{
	if (inValue.empty())
		return false;

#if defined(CHALET_REGEX_CTRE)
	#if defined(CHALET_REGEX_CTRE_20)
	if (auto m = ctre::match<"^(c|gnu)\\+\\+\\d[\\dxyzab]$">(inValue))
	#else
	static constexpr auto regex = ctll::fixed_string{ "^(c|gnu)\\+\\+\\d[\\dxyzab]$" };
	if (auto m = ctre::match<regex>(inValue))
	#endif
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

#if defined(CHALET_REGEX_CTRE)
	#if defined(CHALET_REGEX_CTRE_20)
	if (auto m = ctre::match<"^((c|gnu)\\d[\\dx]|(iso9899:(1990|199409|1999|199x|20\\d{2})))$">(inValue))
	#else
	static constexpr auto regex = ctll::fixed_string{ "^((c|gnu)\\d[\\dx]|(iso9899:(1990|199409|1999|199x|20\\d{2})))$" };
	if (auto m = ctre::match<regex>(inValue))
	#endif
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

#if defined(CHALET_REGEX_CTRE)
	#if defined(CHALET_REGEX_CTRE_20)
	if (auto m = ctre::match<"^\\d[\\dxyzab]$">(inValue))
	#else
	static constexpr auto regex = ctll::fixed_string{ "^\\d[\\dxyzab]$" };
	if (auto m = ctre::match<regex>(inValue))
	#endif
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
bool RegexPatterns::matchesFullVersionString(const std::string& inValue)
{
	if (inValue.empty())
		return false;

		// This pattern doesn't match the number of digits because it's not worth creating a bug over
		//

#if defined(CHALET_REGEX_CTRE)
	#if defined(CHALET_REGEX_CTRE_20)
	if (auto m = ctre::match<"^(\\d+\\.\\d+\\.\\d+\\.\\d+)$">(inValue))
	#else
	static constexpr auto regex = ctll::fixed_string{ "^(\\d+\\.\\d+\\.\\d+\\.\\d+)$" };
	if (auto m = ctre::match<regex>(inValue))
	#endif
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

/*****************************************************************************/
void RegexPatterns::matchConfigureFileVariables(std::string& outText, const std::function<std::string(std::string)>& onMatch)
{
#if defined(CHALET_REGEX_CTRE)
	#if defined(CHALET_REGEX_CTRE_20)
	while (auto m = ctre::match<".*(@(\\w+)@).*">(outText))
	#else
	static constexpr auto regex = ctll::fixed_string{ ".*(@(\\w+)@).*" };
	while (auto m = ctre::match<regex>(outText))
	#endif
	{
		auto whole = m.get<1>().to_string();
		auto replaceValue = onMatch(m.get<2>().to_string());
		String::replaceAll(outText, whole, replaceValue);
	}
#else
	static std::regex re("@(\\w+)@");
	std::smatch sm;
	while (std::regex_search(outText, sm, re))
	{
		auto prefix = sm.prefix().str();
		auto replaceValue = onMatch(sm[1].str());
		auto suffix = sm.suffix().str();
		outText = fmt::format("{}{}{}", prefix, replaceValue, suffix);
	}
#endif
}

/*****************************************************************************/
void RegexPatterns::matchPathVariables(std::string& outText, const std::function<std::string(std::string)>& onMatch)
{
#if defined(CHALET_REGEX_CTRE)
	#if defined(CHALET_REGEX_CTRE_20)
	while (auto m = ctre::match<".*(\\$\\{([\\w:]+)\\}).*">(outText))
	#else
	static constexpr auto regex = ctll::fixed_string{ ".*(\\$\\{([\\w:]+)\\}).*" };
	while (auto m = ctre::match<regex>(outText))
	#endif
	{
		auto whole = m.get<1>().to_string();
		auto replaceValue = onMatch(m.get<2>().to_string());
		String::replaceAll(outText, whole, replaceValue);
	}
#else
	static std::regex re("\\$\\{([\\w:]+)\\}");
	std::smatch sm;
	while (std::regex_search(outText, sm, re))
	{
		auto prefix = sm.prefix().str();
		auto replaceValue = onMatch(sm[1].str());
		auto suffix = sm.suffix().str();
		outText = fmt::format("{}{}{}", prefix, replaceValue, suffix);
	}
#endif
}
}
