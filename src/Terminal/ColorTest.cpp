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
bool ColorTest::run()
{
	// auto printColor = [](const Color inColor) -> std::string {
	// 	auto outColor = Output::getAnsiStyle(inColor);
	// 	auto reset = Output::getAnsiStyle(Color::Reset);
	// 	return fmt::format("{}chalet{}", outColor, reset);
	// };

	// printColor(Color::Reset);

	// using ColorIterator = EnumIterator<Color, Color::Black, Color::White>;
	// using BrightColorIterator = EnumIterator<Color, Color::BrightBlack, Color::BrightWhite>;
	// using BrightBoldColorIterator = EnumIterator<Color, Color::BrightBlackBold, Color::BrightWhiteBold>;

	// StringList lines;
	// for (auto color : ColorIterator())
	// {
	// 	std::string newLine = printColor(color);
	// 	lines.emplace_back(std::move(newLine));
	// }

	// int i = 0;
	// for (auto color : BrightColorIterator())
	// {
	// 	lines[i] += printColor(color);
	// 	++i;
	// }

	// i = 0;
	// for (auto color : BrightBoldColorIterator())
	// {
	// 	lines[i] += printColor(color);
	// 	++i;
	// }

	// std::cout << String::join(lines, '\n') << std::endl;

	// std::cout << std::endl;

	const auto gray = Output::getAnsiStyle(Color::BrightBlack);
	const auto reset = Output::getAnsiStyle(Color::Reset);
	const auto lineBreak = fmt::format("{}{}{}\n", gray, std::string(96, '-'), reset);
	std::cout << lineBreak;

#if defined(CHALET_WIN32)
	char esc = '\x1b';
#else
	char esc = '\033';
#endif

	for (int attr : { 7, 1, 0, 2 })
	{
		for (int clfg : { 30, 90, 37, 97, 31, 91, 32, 92, 33, 93, 34, 94, 35, 95, 36, 96 })
		{
			std::cout << fmt::format("{esc}[{attr};{clfg}m {attr},{clfg} {esc}[0m",
				FMT_ARG(esc),
				FMT_ARG(attr),
				FMT_ARG(clfg));
		}
		std::cout << reset << '\n';
	}

	auto themes = ColorTheme::getAllThemes();
	for (const auto& theme : themes)
	{
		std::string output = lineBreak;
		output += fmt::format("{}:\n\n", theme.preset());
		output += fmt::format("{flair}>  {info}theme.info{flair} ... theme.flair (1ms){reset}\n",
			fmt::arg("flair", Output::getAnsiStyle(theme.flair)),
			fmt::arg("info", Output::getAnsiStyle(theme.info)),
			FMT_ARG(reset));
		output += fmt::format("{}{}  theme.error{}\n", Output::getAnsiStyle(theme.error), Unicode::heavyBallotX(), reset);
		output += fmt::format("{}{}  theme.warning{}\n", Output::getAnsiStyle(theme.warning), Unicode::warning(), reset);
		output += fmt::format("{}{}  theme.success{}\n", Output::getAnsiStyle(theme.success), Unicode::heavyCheckmark(), reset);
		output += fmt::format("{}{}  theme.note{}\n", Output::getAnsiStyle(theme.note), Unicode::diamond(), reset);
		output += fmt::format("{}{}  theme.header{}\n", Output::getAnsiStyle(theme.header), Unicode::triangle(), reset);
		output += fmt::format("   [1/1] {}theme.build{}\n", Output::getAnsiStyle(theme.build), reset);
		output += fmt::format("   [1/1] {}theme.assembly{}\n\n", Output::getAnsiStyle(theme.assembly), reset);

		std::cout << output;
	}

	std::cout << lineBreak << std::endl;

	return true;
}
}