/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Terminal/ColorTheme.hpp"

#include "Terminal/Environment.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"

namespace chalet
{
namespace
{
OrderedDictionary<Color> kThemeMap{
	{ "reset", Color::Reset },
	//
	{ "black", Color::Black },
	{ "red", Color::Red },
	{ "green", Color::Green },
	{ "yellow", Color::Yellow },
	{ "blue", Color::Blue },
	{ "magenta", Color::Magenta },
	{ "cyan", Color::Cyan },
	{ "white", Color::White },
	//
	{ "brightBlack", Color::BrightBlack },
	{ "brightRed", Color::BrightRed },
	{ "brightGreen", Color::BrightGreen },
	{ "brightYellow", Color::BrightYellow },
	{ "brightBlue", Color::BrightBlue },
	{ "brightMagenta", Color::BrightMagenta },
	{ "brightCyan", Color::BrightCyan },
	{ "brightWhite", Color::BrightWhite },
	//
	{ "brightBlackBold", Color::BrightBlackBold },
	{ "brightRedBold", Color::BrightRedBold },
	{ "brightGreenBold", Color::BrightGreenBold },
	{ "brightYellowBold", Color::BrightYellowBold },
	{ "brightBlueBold", Color::BrightBlueBold },
	{ "brightMagentaBold", Color::BrightMagentaBold },
	{ "brightCyanBold", Color::BrightCyanBold },
	{ "brightWhiteBold", Color::BrightWhiteBold },
};

StringList kPresetNames{
	"default"
};
}

/*****************************************************************************/
Color ColorTheme::getColorFromKey(const std::string& inString)
{
	if (kThemeMap.find(inString) != kThemeMap.end())
		return kThemeMap.at(inString);

	return Color::Reset;
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
StringList ColorTheme::getJsonColors()
{
	StringList ret;
	for (auto& [id, _] : kThemeMap)
	{
		ret.push_back(id);
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
		"answer",
		"alt",
		"note",
	};
}

/*****************************************************************************/
const std::string& ColorTheme::defaultPresetName()
{
	return kPresetNames.front();
}

/*****************************************************************************/
bool ColorTheme::isValidPreset(const std::string& inPresetName)
{
	if (inPresetName.empty())
		return false;

	return List::contains(kPresetNames, inPresetName);
}

/*****************************************************************************/
bool ColorTheme::set(const std::string& inKey, const std::string& inValue)
{
	Color* color = getColorFromString(inKey);
	if (color != nullptr)
	{
		*color = getColorFromKey(inValue);
		return true;
	}

	return false;
}

/*****************************************************************************/
std::string ColorTheme::getAsString(const std::string& inKey) const
{
	const Color* color = getColorFromString(inKey);
	if (color != nullptr)
		return ColorTheme::getStringFromColor(*color);

	return std::string();
}

/*****************************************************************************/
std::string ColorTheme::asString() const
{
	if (isPreset())
		return m_preset;

	return fmt::format("{} {} {} {} {} {} {} {} {} {}",
		static_cast<ushort>(error),
		static_cast<ushort>(warning),
		static_cast<ushort>(success),
		static_cast<ushort>(flair),
		static_cast<ushort>(info),
		static_cast<ushort>(header),
		static_cast<ushort>(build),
		static_cast<ushort>(alt),
		static_cast<ushort>(answer),
		static_cast<ushort>(note));
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
		&& alt == rhs.answer
		&& alt == rhs.alt
		&& note == rhs.note
		&& m_preset == rhs.m_preset;
}

bool ColorTheme::operator!=(const ColorTheme& rhs) const
{
	return !operator==(rhs);
}

/*****************************************************************************/
const std::string& ColorTheme::preset() const noexcept
{
	return m_preset;
}

void ColorTheme::setPreset(const std::string& inValue)
{
	if (!List::contains(kPresetNames, inValue))
		return;

	m_preset = inValue;

	makePreset(inValue);
}

bool ColorTheme::isPreset() const noexcept
{
	return !m_preset.empty();
}

/*****************************************************************************/
void ColorTheme::makePreset(const std::string& inValue)
{
	if (String::equals(kPresetNames.at(0), inValue))
	{
		info = Color::BrightWhite;
		error = Color::BrightRedBold;
		warning = Color::BrightYellowBold;
		success = Color::BrightGreenBold;
		flair = Color::BrightBlack;
		header = Color::BrightYellowBold;
		build = Color::BrightBlue;
		answer = Color::BrightMagentaBold;
		alt = Color::Magenta;
		note = Color::Blue;
	}
}

/*****************************************************************************/

#define GET_COLORS(inKey)                      \
	if (String::equals("info", inKey))         \
		return &info;                          \
	else if (String::equals("error", inKey))   \
		return &error;                         \
	else if (String::equals("warning", inKey)) \
		return &warning;                       \
	else if (String::equals("success", inKey)) \
		return &success;                       \
	else if (String::equals("flair", inKey))   \
		return &flair;                         \
	else if (String::equals("header", inKey))  \
		return &header;                        \
	else if (String::equals("build", inKey))   \
		return &build;                         \
	else if (String::equals("answer", inKey))  \
		return &answer;                        \
	else if (String::equals("alt", inKey))     \
		return &alt;                           \
	else if (String::equals("note", inKey))    \
		return &note;                          \
	return nullptr

Color* ColorTheme::getColorFromString(const std::string& inKey)
{
	GET_COLORS(inKey);
}

const Color* ColorTheme::getColorFromString(const std::string& inKey) const
{
	GET_COLORS(inKey);
}

#undef GET_COLORS

}
