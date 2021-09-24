/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Terminal/ColorTheme.hpp"

#include "Terminal/Environment.hpp"
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
	if (Environment::isContinuousIntegrationServer())
	{
		// Black might be unreadable (Github Actions anyway)
		flair = Color::Reset;
	}
}

/*****************************************************************************/
bool ColorTheme::parseColorFromString(const std::string& inString, Color& outColor)
{
	if (String::equals("reset", inString))
	{
		outColor = Color::Reset;
	}
	else if (String::equals("black", inString))
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
	else
	{
		return false;
	}

	return true;
}

/*****************************************************************************/
std::string ColorTheme::getStringFromColor(const Color inColor)
{
	std::string ret;
	switch (inColor)
	{
		case Color::Reset:
			ret = "reset";
			break;

		case Color::Black:
			ret = "black";
			break;

		case Color::Red:
			ret = "red";
			break;

		case Color::Green:
			ret = "green";
			break;

		case Color::Yellow:
			ret = "yellow";
			break;

		case Color::Blue:
			ret = "blue";
			break;

		case Color::Magenta:
			ret = "magenta";
			break;

		case Color::Cyan:
			ret = "cyan";
			break;

		case Color::White:
			ret = "white";
			break;
	}

	return ret;
}

/*****************************************************************************/
StringList ColorTheme::getKeys()
{
	return {
		"info",
		"error",
		"warning",
		"success",
		"flair",
		"header",
		"build",
		"alt",
		"note",
	};
}

/*****************************************************************************/
bool ColorTheme::set(const std::string& inKey, const std::string& inValue)
{
	Color* color = getColorFromString(inKey);
	if (color != nullptr)
		return parseColorFromString(inValue, *color);

	return false;
}

/*****************************************************************************/
std::string ColorTheme::getAsString(const std::string& inKey)
{

	const Color* color = getColorFromString(inKey);
	if (color != nullptr)
		return ColorTheme::getStringFromColor(*color);

	return std::string();
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

/*****************************************************************************/
bool ColorTheme::operator==(const ColorTheme& rhs) const
{
	return error == rhs.error
		&& warning == rhs.warning
		&& success == rhs.success
		&& flair == rhs.flair
		&& info == rhs.info
		&& header == rhs.header
		&& build == rhs.build
		&& alt == rhs.alt
		&& note == rhs.note;
}
bool ColorTheme::operator!=(const ColorTheme& rhs) const
{
	return !operator==(rhs);
}

/*****************************************************************************/
Color* ColorTheme::getColorFromString(const std::string& inKey)
{
	Color* color = nullptr;

	if (String::equals("info", inKey))
	{
		color = &info;
	}
	else if (String::equals("error", inKey))
	{
		color = &error;
	}
	else if (String::equals("warning", inKey))
	{
		color = &warning;
	}
	else if (String::equals("success", inKey))
	{
		color = &success;
	}
	else if (String::equals("flair", inKey))
	{
		color = &flair;
	}
	else if (String::equals("header", inKey))
	{
		color = &header;
	}
	else if (String::equals("build", inKey))
	{
		color = &build;
	}
	else if (String::equals("alt", inKey))
	{
		color = &alt;
	}
	else if (String::equals("note", inKey))
	{
		color = &note;
	}

	return color;
}
}
