/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Terminal/ColorTest.hpp"

#include "Terminal/Color.hpp"
#include "Terminal/Output.hpp"
#include "Utility/EnumIterator.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
bool ColorTest::run()
{
	auto printColor = [](const Color inColor) -> std::string {
		auto outColor = Output::getAnsiStyle(inColor);
		auto reset = Output::getAnsiStyle(Color::Reset);
		return fmt::format("{}CHALET chalet {}", outColor, reset);
	};

	printColor(Color::Reset);

	using ColorIterator = EnumIterator<Color, Color::Black, Color::White>;
	using BrightColorIterator = EnumIterator<Color, Color::BrightBlack, Color::BrightWhite>;
	using BrightBoldColorIterator = EnumIterator<Color, Color::BrightBlackBold, Color::BrightWhiteBold>;

	StringList lines;
	for (auto color : ColorIterator())
	{
		std::string newLine = printColor(color);
		lines.emplace_back(std::move(newLine));
	}

	int i = 0;
	for (auto color : BrightColorIterator())
	{
		lines[i] += printColor(color);
		++i;
	}

	i = 0;
	for (auto color : BrightBoldColorIterator())
	{
		lines[i] += printColor(color);
		++i;
	}

	std::cout << String::join(lines, '\n') << std::endl;
	return true;
}
}
