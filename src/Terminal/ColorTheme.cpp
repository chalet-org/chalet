/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Terminal/ColorTheme.hpp"

#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
ColorTheme::ColorTheme() :
	info(Color::Reset),
	error(Color::Red),
	warning(Color::Yellow),
	success(Color::Green),
	flair(Color::Black),
	header(Color::Yellow),
	build(Color::Blue),
	alt(Color::Magenta),
	note(Color::Blue)
{
}

/*****************************************************************************/
bool ColorTheme::parseColorFromString(const std::string& inString, Color& outColor)
{
	if (String::equals("black", inString))
	{
		outColor = Color::Black;
	}
	else if (String::equals("red", inString))
	{
		outColor = Color::Red;
	}
	else if (String::equals("green", inString))
	{
		outColor = Color::Green;
	}
	else if (String::equals("yellow", inString))
	{
		outColor = Color::Yellow;
	}
	else if (String::equals("blue", inString))
	{
		outColor = Color::Blue;
	}
	else if (String::equals("magenta", inString))
	{
		outColor = Color::Magenta;
	}
	else if (String::equals("cyan", inString))
	{
		outColor = Color::Cyan;
	}
	else if (String::equals("white", inString))
	{
		outColor = Color::White;
	}
	else if (String::equals("reset", inString))
	{
		outColor = Color::Reset;
	}
	else
	{
		return false;
	}

	return true;
}

/*****************************************************************************/
std::string ColorTheme::asString() const
{
	return fmt::format("{} {} {} {} {} {} {} {} {}",
		static_cast<short>(error),
		static_cast<short>(warning),
		static_cast<short>(success),
		static_cast<short>(flair),
		static_cast<short>(info),
		static_cast<short>(header),
		static_cast<short>(build),
		static_cast<short>(alt),
		static_cast<short>(note));
}

}
