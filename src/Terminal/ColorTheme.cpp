/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Terminal/ColorTheme.hpp"

#include "Terminal/Environment.hpp"
#include "Utility/String.hpp"

namespace chalet
{
namespace
{
Dictionary<Color> kThemeMap{
	{ "reset", Color::Reset },
	{ "black", Color::Black },
	{ "red", Color::Red },
	{ "green", Color::Green },
	{ "yellow", Color::Yellow },
	{ "blue", Color::Blue },
	{ "magenta", Color::Magenta },
	{ "cyan", Color::Cyan },
	{ "white", Color::White },
};
}
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
	if (kThemeMap.find(inString) != kThemeMap.end())
	{
		outColor = kThemeMap.at(inString);
		return true;
	}
	else
	{
		return false;
	}
}

/*****************************************************************************/
std::string ColorTheme::getStringFromColor(const Color inColor)
{
	for (auto& [id, color] : kThemeMap)
	{
		if (inColor == color)
			return id;
	}

	return std::string();
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
