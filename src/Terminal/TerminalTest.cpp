/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Terminal/TerminalTest.hpp"

#include "Terminal/Output.hpp"
#include "Terminal/Unicode.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
TerminalTest::TerminalTest() :
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
bool TerminalTest::run()
{
	m_gray = Output::getAnsiStyle(Color::BrightBlack);
	m_reset = Output::getAnsiStyle(Color::Reset);
	m_white = Output::getAnsiStyle(Color::BrightWhiteBold);
	m_separator = fmt::format("{}{}{}\n", m_gray, kSeparator, m_reset);

	auto output = fmt::format("{esc}[2J{esc}[1;1H", fmt::arg("esc", kEsc));
	std::cout.write(output.data(), output.size());
	std::cout.flush();

#if defined(CHALET_DEBUG)
	// printColorCombinations();
	printChaletColorThemes(false);
#else
	printChaletColorThemes(false);
#endif

	printUnicodeCharacters();
	printTerminalCapabilities();

	std::cout.write(m_separator.data(), m_separator.size());
	std::cout.flush();

	return true;
}

/*****************************************************************************/
void TerminalTest::printTerminalCapabilities()
{
	printBanner("Terminal Capabilities");
	std::cout.write(m_separator.data(), m_separator.size());

	std::unordered_map<i32, std::string> descriptions{
		{ 0, "normal" },
		{ 1, "bold" },
		{ 2, "dim" },
		{ 3, "italic" },
		{ 4, "underlined" },
		{ 5, "blink" },
		{ 7, "inverted" },
		{ 9, "strikethrough" },
	};

	for (i32 attr : { 7, 1, 0, 2, 3, 4, 9, 5 })
	{
		for (i32 clfg : { 30, 90, 37, 97, 33, 93, 31, 91, 35, 95, 34, 94, 36, 96, 32, 92 })
		{
			auto output = fmt::format("{esc}[{attr};{clfg}m {clfg} {esc}[0m",
				fmt::arg("esc", kEsc),
				FMT_ARG(attr),
				FMT_ARG(clfg));

			std::cout.write(output.data(), output.size());
		}
		if (descriptions.find(attr) != descriptions.end())
		{
			auto output = fmt::format("{} - {}({}) {}\n", m_gray, m_reset, attr, descriptions.at(attr));
			std::cout.write(output.data(), output.size());
		}
		else
		{
			std::cout.write(m_reset.data(), m_reset.size());
			std::cout.put('\n');
		}
	}

	std::cout.write(m_separator.data(), m_separator.size());

	{
		std::string label;
		i32 attr = 7;
		for (i32 clfg : { 30, 37, 33, 31, 35, 34, 36, 32 })
		{
			auto output = fmt::format("{esc}[{attr};{clfg}m        {esc}[0m",
				fmt::arg("esc", kEsc),
				FMT_ARG(attr),
				FMT_ARG(clfg));

			std::cout.write(output.data(), output.size());
		}

		label = fmt::format("{} - {}(3x) normal\n", m_gray, m_reset);
		std::cout.write(label.data(), label.size());

		for (i32 clfg : { 90, 97, 93, 91, 95, 94, 96, 92 })
		{
			auto output = fmt::format("{esc}[{attr};{clfg}m        {esc}[0m",
				fmt::arg("esc", kEsc),
				FMT_ARG(attr),
				FMT_ARG(clfg));

			std::cout.write(output.data(), output.size());
		}
		label = fmt::format("{} - {}(9x) bright\n", m_gray, m_reset);
		std::cout.write(label.data(), label.size());
	}
}

/*****************************************************************************/
void TerminalTest::printUnicodeCharacters()
{
	StringList characters;
	characters.emplace_back(Unicode::triangle());
	characters.emplace_back(Unicode::diamond());
	characters.emplace_back(Unicode::checkmark());
	characters.emplace_back(Unicode::heavyBallotX());
	characters.emplace_back(Unicode::heavyCurvedDownRightArrow());
	characters.emplace_back(Unicode::registered());

	printBanner("Supported Unicode Characters");

	const auto& bold = Output::getAnsiStyle(Color::WhiteBold);

	std::string output = m_separator + bold;
	for (auto& character : characters)
	{
		output += fmt::format("{}  ", character);
	}
	output += fmt::format("{}\n", m_reset);
	std::cout.write(output.data(), output.size());
}

/*****************************************************************************/
void TerminalTest::printChaletColorThemes(const bool inSimple)
{
	auto themes = ColorTheme::getAllThemes();
	size_t totalThemes = themes.size();

	printBanner("Chalet Color Themes");

	bool printName = true;
	if (inSimple)
	{
		for (const auto& theme : themes)
			printThemeSimple(theme, printName);
	}
	else
	{
		for (const auto& theme : themes)
			printTheme(theme, printName);
	}

	auto total = fmt::format("Total built-in themes: {}\n", totalThemes);

	std::cout.write(m_separator.data(), m_separator.size());
	std::cout.write(total.data(), total.size());

	auto& currentTheme = Output::theme();
	if (currentTheme.preset().empty())
	{
		printTheme(currentTheme, printName);
	}
}

/*****************************************************************************/
void TerminalTest::printColorCombinations()
{
	printBanner("Color Combinations");
	std::cout.write(m_separator.data(), m_separator.size());

	std::vector<i32> headers{ 33, 93, 31, 91, 35, 95, 34, 94, 36, 96, 32, 92 };
	std::vector<i32> colors{ 33, 93, 31, 91, 35, 95, 34, 94, 36, 96, 32, 92 };
	// std::vector<i32> flairs{ 90, 233, 293, 231, 291, 235, 295, 234, 294, 236, 296, 232, 292 };
	i32 flair = 90;
	i32 assembly = 90;
	i32 success = 92;
	// for (i32 flair : flairs)
	for (i32 build : colors)
		for (i32 header : headers)
		// for (i32 assembly : colors)
		// for (i32 success : colors)
		{
			// printColored("doot", clfg, attr);
			ColorTheme theme;
			theme.info = Color::Reset;
			theme.flair = static_cast<Color>(flair);
			theme.header = static_cast<Color>(100 + header);
			theme.build = static_cast<Color>(build);
			theme.assembly = static_cast<Color>(assembly);
			theme.success = static_cast<Color>(100 + success);
			printThemeSimple(theme);
		}

	std::cout.write(m_reset.data(), m_reset.size());
	std::cout.put('\n');

	std::cout.write(m_separator.data(), m_separator.size());
}

/*****************************************************************************/
void TerminalTest::printBanner(const std::string& inText)
{
	auto middle = static_cast<size_t>(static_cast<f64>(kWidth) * 0.5);
	auto textMiddle = static_cast<size_t>(static_cast<f64>(inText.size()) * 0.5);
	std::string padding(middle - textMiddle, ' ');

	auto output = fmt::format("{}{}{}\n", m_separator, padding, inText);

	std::cout.write(output.data(), output.size());
}

/*****************************************************************************/
void TerminalTest::printTheme(const ColorTheme& theme, const bool inWithName)
{
	std::string output = m_separator;
	if (inWithName)
	{
		std::string name = theme.preset().empty() ? "(custom)" : theme.preset();
		auto hex = theme.asHexString();
		output += fmt::format("{m_gray}:: {m_white}{name} {m_gray}:: {hex}{m_reset}\n\n",
			FMT_ARG(m_gray),
			FMT_ARG(m_white),
			FMT_ARG(name),
			FMT_ARG(hex),
			FMT_ARG(m_reset));
	}

	output += fmt::format("{flair}>  {info}theme.info{flair} ... theme.flair (1ms){m_reset}\n",
		fmt::arg("flair", Output::getAnsiStyle(theme.flair)),
		fmt::arg("info", Output::getAnsiStyle(theme.info)),
		FMT_ARG(m_reset));
	output += fmt::format("{}{}  theme.header{}\n", Output::getAnsiStyle(theme.header), Unicode::triangle(), m_reset);
	output += fmt::format("   [1/1] {}theme.build{}\n", Output::getAnsiStyle(theme.build), m_reset);
	output += fmt::format("   [1/1] {}theme.assembly{}\n", Output::getAnsiStyle(theme.assembly), m_reset);
	output += fmt::format("{}{}  theme.success{}\n", Output::getAnsiStyle(theme.success), Unicode::checkmark(), m_reset);
	output += fmt::format("{}{}  theme.error{}\n", Output::getAnsiStyle(theme.error), Unicode::heavyBallotX(), m_reset);
	output += fmt::format("{}{}  theme.warning{}\n", Output::getAnsiStyle(theme.warning), Unicode::warning(), m_reset);
	output += fmt::format("{}{}  theme.note{}\n", Output::getAnsiStyle(theme.note), Unicode::diamond(), m_reset);

	std::cout.write(output.data(), output.size());
}

/*****************************************************************************/
void TerminalTest::printThemeSimple(const ColorTheme& theme, const bool inWithName)
{
	auto makeString = [](const Color& color) {
		auto value = std::to_string(static_cast<i32>(color));
		while (value.size() < 3)
			value += ' ';

		return value;
	};

	std::string output = m_separator;
	if (inWithName)
	{
		std::string name = theme.preset().empty() ? "(custom)" : theme.preset();
		output += fmt::format("{m_gray}:: {m_white}{name} {m_gray}::{m_reset}\n",
			FMT_ARG(m_gray),
			FMT_ARG(m_white),
			FMT_ARG(name),
			FMT_ARG(m_reset));
	}
	output += fmt::format("{id} | {flair}>  {info}theme.info{flair} ... theme.flair (1ms){m_reset}\n",
		fmt::arg("id", makeString(theme.flair)),
		fmt::arg("flair", Output::getAnsiStyle(theme.flair)),
		fmt::arg("info", Output::getAnsiStyle(theme.info)),
		FMT_ARG(m_reset));
	output += fmt::format("{} | {}{}  theme.header{}\n", makeString(theme.header), Output::getAnsiStyle(theme.header), Unicode::triangle(), m_reset);
	output += fmt::format("{} |    [1/1] {}theme.build{}\n", makeString(theme.build), Output::getAnsiStyle(theme.build), m_reset);
	// output += fmt::format("   [1/1] {}theme.assembly{}\n", Output::getAnsiStyle(theme.assembly), m_reset);
	output += fmt::format("{} | {}{}  theme.success{}\n", makeString(theme.success), Output::getAnsiStyle(theme.success), Unicode::checkmark(), m_reset);
	// output += fmt::format("{}{}  theme.error{}\n", Output::getAnsiStyle(theme.error), Unicode::heavyBallotX(), m_reset);
	// output += fmt::format("{}{}  theme.warning{}\n", Output::getAnsiStyle(theme.warning), Unicode::warning(), m_reset);
	// output += fmt::format("{}{}  theme.note{}\n", Output::getAnsiStyle(theme.note), Unicode::diamond(), m_reset);

	std::cout.write(output.data(), output.size());
}
}
