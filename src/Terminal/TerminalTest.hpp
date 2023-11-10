/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include "Terminal/ColorTheme.hpp"

namespace chalet
{
struct TerminalTest
{
	TerminalTest();

	bool run();

private:
	void printTerminalCapabilities();
	void printUnicodeCharacters();
	void printChaletColorThemes(const bool inSimple = false);
	void printColorCombinations();

	void printBanner(const std::string& inText);
	void printTheme(const ColorTheme& inTheme, const bool inWithName = false);
	void printThemeSimple(const ColorTheme& inTheme, const bool inWithName = false);

	const char kEsc;
	const size_t kWidth;

	const std::string kSeparator;

	std::string m_gray;
	std::string m_reset;
	std::string m_white;
	std::string m_separator;
};
}
