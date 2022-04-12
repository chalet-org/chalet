/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Utility/RegexPatterns.hpp"

#include <regex>

namespace chalet
{
/*****************************************************************************/
bool RegexPatterns::matchesGnuCppStandard(const std::string& inValue)
{
	if (inValue.empty())
		return false;

	static std::regex regex{ "^(c|gnu)\\+\\+\\d[\\dxyzab]$" };
	if (std::regex_match(inValue, regex))
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

	static std::regex regex{ "^((c|gnu)\\d[\\dx]|(iso9899:(1990|199409|1999|199x|20\\d{2})))$" };
	if (std::regex_match(inValue, regex))
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

	static std::regex regex{ "^\\d[\\dxyzab]$" };
	if (std::regex_match(inValue, regex))
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

	static std::regex regex{ "^(\\d+\\.\\d+\\.\\d+\\.\\d+)$" };
	if (std::regex_match(inValue, regex))
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
bool RegexPatterns::matchPathVariables(std::string& outText, const std::function<std::string(std::string)>& onMatch)
{
	static std::regex re(R"regex(\$\{(\w+)\})regex");

	std::smatch sm;
	while (std::regex_search(outText, sm, re))
	{
		auto prefix = sm.prefix().str();
		auto replaceValue = onMatch(sm[1].str());
		auto suffix = sm.suffix().str();
		outText = fmt::format("{}{}{}", prefix, replaceValue, suffix);
	}

	return true;
}
}
