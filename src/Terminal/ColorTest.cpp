/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Terminal/ColorTest.hpp"

#include "Terminal/Color.hpp"
#include "Terminal/Diagnostic.hpp"
#include "Terminal/Output.hpp"
#include "Terminal/Unicode.hpp"
#include "Utility/EnumIterator.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
ColorTest::ColorTest() :
#if defined(CHALET_WIN32)
	kEsc('\x1b'),
#else
	kEsc('\033'),
#endif
	kWidth(64),
	kSeparator(kWidth, '-')
{
}

/*****************************************************************************/
bool ColorTest::run()
{
	m_gray = Output::getAnsiStyle(Color::BrightBlack);
	m_reset = Output::getAnsiStyle(Color::Reset);
	m_white = Output::getAnsiStyle(Color::BrightWhiteBold);
	m_separator = fmt::format("{}{}{}\n", m_gray, kSeparator, m_reset);

	std::cout << fmt::format("{esc}[2J{esc}[1;1H", fmt::arg("esc", kEsc));

	printChaletColorThemes();
	printTerminalCapabilities();

	std::cout << m_separator << std::flush;

	return true;
}

/*****************************************************************************/
void ColorTest::printTerminalCapabilities()
{
	std::cout << m_separator << std::string(22, ' ') << "Terminal Capabilities\n"
			  << m_separator;

	std::unordered_map<int, std::string> descriptions{
		{ 0, "normal" },
		{ 1, "bold" },
		{ 2, "dim" },
		{ 3, "italic" },
		{ 4, "underlined" },
		{ 5, "blink" },
		{ 7, "inverted" },
		{ 9, "strikethrough" },
	};

	for (int attr : { 7, 1, 0, 2, 3, 4, 9, 5 })
	{
		for (int clfg : { 30, 90, 37, 97, 33, 93, 31, 91, 35, 95, 34, 94, 36, 96, 32, 92 })
		{
			std::cout << fmt::format("{esc}[{attr};{clfg}m {clfg} {esc}[0m",
				fmt::arg("esc", kEsc),
				FMT_ARG(attr),
				FMT_ARG(clfg));
		}
		if (descriptions.find(attr) != descriptions.end())
		{
			std::cout << fmt::format("{} - {}({}) {}\n", m_gray, m_reset, attr, descriptions.at(attr));
		}
		else
		{
			std::cout << m_reset << '\n';
		}
	}

	std::cout << m_separator;

	{
		int attr = 7;
		for (int clfg : { 30, 37, 33, 31, 35, 34, 36, 32 })
		{
			std::cout << fmt::format("{esc}[{attr};{clfg}m        {esc}[0m",
				fmt::arg("esc", kEsc),
				FMT_ARG(attr),
				FMT_ARG(clfg));
		}
		std::cout << fmt::format("{} - {}(3x) normal\n", m_gray, m_reset);

		for (int clfg : { 90, 97, 93, 91, 95, 94, 96, 92 })
		{
			std::cout << fmt::format("{esc}[{attr};{clfg}m        {esc}[0m",
				fmt::arg("esc", kEsc),
				FMT_ARG(attr),
				FMT_ARG(clfg));
		}
		std::cout << fmt::format("{} - {}(9x) bright\n", m_gray, m_reset);
	}
}

/*****************************************************************************/
void ColorTest::printChaletColorThemes()
{
	auto currentTheme = Output::theme();
	auto themes = ColorTheme::getAllThemes();
	std::size_t totalThemes = themes.size();
	if (currentTheme.preset().empty())
		themes.push_back(std::move(currentTheme));

	std::cout << m_separator << std::string(23, ' ') << "Chalet Color Themes\n";

	for (const auto& theme : themes)
	{
		std::string output = m_separator;
		std::string name = theme.preset().empty() ? "(custom)" : theme.preset();
		output += fmt::format("{m_gray}:: {m_white}{name} {m_gray}::{m_reset}\n\n",
			FMT_ARG(m_gray),
			FMT_ARG(m_white),
			FMT_ARG(name),
			FMT_ARG(m_reset));
		output += fmt::format("{flair}>  {info}theme.info{flair} ... theme.flair (1ms){m_reset}\n",
			fmt::arg("flair", Output::getAnsiStyle(theme.flair)),
			fmt::arg("info", Output::getAnsiStyle(theme.info)),
			FMT_ARG(m_reset));
		output += fmt::format("{}{}  theme.header{}\n", Output::getAnsiStyle(theme.header), Unicode::triangle(), m_reset);
		output += fmt::format("   [1/1] {}theme.build{}\n", Output::getAnsiStyle(theme.build), m_reset);
		output += fmt::format("   [1/1] {}theme.assembly{}\n", Output::getAnsiStyle(theme.assembly), m_reset);
		output += fmt::format("{}{}  theme.success{}\n", Output::getAnsiStyle(theme.success), Unicode::heavyCheckmark(), m_reset);
		output += fmt::format("{}{}  theme.error{}\n", Output::getAnsiStyle(theme.error), Unicode::heavyBallotX(), m_reset);
		output += fmt::format("{}{}  theme.warning{}\n", Output::getAnsiStyle(theme.warning), Unicode::warning(), m_reset);
		output += fmt::format("{}{}  theme.note{}\n", Output::getAnsiStyle(theme.note), Unicode::diamond(), m_reset);

		std::cout << output;
	}

	std::cout << m_separator << fmt::format("Total built-in themes: {}\n", totalThemes);
}
}
