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
static struct
{
	const OrderedDictionary<Color> themeMap{
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
		{ "blackBold", Color::BlackBold },
		{ "redBold", Color::RedBold },
		{ "greenBold", Color::GreenBold },
		{ "yellowBold", Color::YellowBold },
		{ "blueBold", Color::BlueBold },
		{ "magentaBold", Color::MagentaBold },
		{ "cyanBold", Color::CyanBold },
		{ "whiteBold", Color::WhiteBold },
		//
		{ "brightBlackBold", Color::BrightBlackBold },
		{ "brightRedBold", Color::BrightRedBold },
		{ "brightGreenBold", Color::BrightGreenBold },
		{ "brightYellowBold", Color::BrightYellowBold },
		{ "brightBlueBold", Color::BrightBlueBold },
		{ "brightMagentaBold", Color::BrightMagentaBold },
		{ "brightCyanBold", Color::BrightCyanBold },
		{ "brightWhiteBold", Color::BrightWhiteBold },
		//
		{ "blackDim", Color::BlackDim },
		{ "redDim", Color::RedDim },
		{ "greenDim", Color::GreenDim },
		{ "yellowDim", Color::YellowDim },
		{ "blueDim", Color::BlueDim },
		{ "magentaDim", Color::MagentaDim },
		{ "cyanDim", Color::CyanDim },
		{ "whiteDim", Color::WhiteDim },
		//
		{ "brightBlackDim", Color::BrightBlackDim },
		{ "brightRedDim", Color::BrightRedDim },
		{ "brightGreenDim", Color::BrightGreenDim },
		{ "brightYellowDim", Color::BrightYellowDim },
		{ "brightBlueDim", Color::BrightBlueDim },
		{ "brightMagentaDim", Color::BrightMagentaDim },
		{ "brightCyanDim", Color::BrightCyanDim },
		{ "brightWhiteDim", Color::BrightWhiteDim },
		//
		{ "blackInverted", Color::BlackInverted },
		{ "redInverted", Color::RedInverted },
		{ "greenInverted", Color::GreenInverted },
		{ "yellowInverted", Color::YellowInverted },
		{ "blueInverted", Color::BlueInverted },
		{ "magentaInverted", Color::MagentaInverted },
		{ "cyanInverted", Color::CyanInverted },
		{ "whiteInverted", Color::WhiteInverted },
		//
		{ "brightBlackInverted", Color::BrightBlackInverted },
		{ "brightRedInverted", Color::BrightRedInverted },
		{ "brightGreenInverted", Color::BrightGreenInverted },
		{ "brightYellowInverted", Color::BrightYellowInverted },
		{ "brightBlueInverted", Color::BrightBlueInverted },
		{ "brightMagentaInverted", Color::BrightMagentaInverted },
		{ "brightCyanInverted", Color::BrightCyanInverted },
		{ "brightWhiteInverted", Color::BrightWhiteInverted },
	};

	const StringList presetNames{
		"default",
		"none",
		"palapa",
		"highrise",
		"teahouse",
		"skilodge",
		"temple",
		"bungalow",
		"cottage",
		"monastery",
		"longhouse",
		"greenhouse",
		"observatory",
		"yurt"
	};
} state;
}

/*****************************************************************************/
Color getColorFromDigit(char value)
{
	if (value >= 97)
		value -= 87;
	else if (value >= 65)
		value -= 55;
	else if (value >= 48)
		value -= 48;

	switch (value)
	{
		case 1: return Color::BrightBlack;
		case 2: return Color::Yellow;
		case 3: return Color::BrightYellow;
		case 4: return Color::Red;
		case 5: return Color::BrightRed;
		case 6: return Color::Magenta;
		case 7: return Color::BrightMagenta;
		case 8: return Color::Blue;
		case 9: return Color::BrightBlue;
		case 10: return Color::Cyan;
		case 11: return Color::BrightCyan;
		case 12: return Color::Green;
		case 13: return Color::BrightGreen;
		case 14: return Color::White;
		case 15: return Color::BrightWhite;
		default:
			return Color::Reset;
	}
}
Color getColorFromDigit(const char inValue, const int inOffset)
{
	auto color = getColorFromDigit(inValue);
	if (color == Color::Black || color == Color::Reset)
		return color;

	return static_cast<Color>(inOffset + static_cast<int>(color));
}

/*****************************************************************************/
ColorTheme ColorTheme::fromHex(const std::string& digits)
{
	ColorTheme theme;

	auto size = digits.size();
	if (size >= 1)
	{
		theme.reset = Color::Reset;
		theme.flair = getColorFromDigit(digits[0], 200);

		if (theme.flair == Color::BrightBlackDim || theme.flair == Color::WhiteDim || theme.flair == Color::BrightWhiteDim)
			theme.flair = Color::BrightBlack;
	}

	if (size >= 2)
		theme.header = getColorFromDigit(digits[1], 100);

	if (size >= 3)
		theme.build = getColorFromDigit(digits[2]);

	if (size >= 4)
		theme.assembly = getColorFromDigit(digits[3]);

	if (size >= 5)
		theme.success = getColorFromDigit(digits[4], 100);

	if (size >= 6)
		theme.error = getColorFromDigit(digits[5], 100);

	if (size >= 7)
		theme.warning = getColorFromDigit(digits[6], 100);

	if (size >= 8)
		theme.note = getColorFromDigit(digits[7], 100);

	// if (size >= 9)
	// 	theme.info = getColorFromDigit(digits[8]);

	theme.info = Color::Reset;

	return theme;
}

/*****************************************************************************/
Color ColorTheme::getColorFromKey(const std::string& inString)
{
	if (state.themeMap.find(inString) != state.themeMap.end())
		return state.themeMap.at(inString);

	return Color::Reset;
}

/*****************************************************************************/
std::string ColorTheme::getStringFromColor(const Color inColor)
{
	for (auto& [id, color] : state.themeMap)
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
	for (auto& [id, _] : state.themeMap)
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
		"note",
		"flair",
		"header",
		"build",
		"assembly",
	};
}

/*****************************************************************************/
const std::string& ColorTheme::defaultPresetName()
{
	return state.presetNames.front();
}

const std::string& ColorTheme::lastPresetName()
{
	return state.presetNames.back();
}

const StringList& ColorTheme::presets()
{
	return state.presetNames;
}

/*****************************************************************************/
bool ColorTheme::isValidPreset(const std::string& inPresetName)
{
	if (inPresetName.empty())
		return false;

	return List::contains(state.presetNames, inPresetName);
}

/*****************************************************************************/
std::vector<ColorTheme> ColorTheme::getAllThemes()
{
	std::vector<ColorTheme> ret;

	for (auto& preset : state.presetNames)
	{
		ret.emplace_back(preset);
	}

	return ret;
}

/*****************************************************************************/
ColorTheme::ColorTheme(const std::string& inPresetName)
{
	setPreset(inPresetName);
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

	using ColorType = std::underlying_type_t<Color>;

	return fmt::format("{} {} {} {} {} {} {} {} {}",
		static_cast<ColorType>(info),
		static_cast<ColorType>(error),
		static_cast<ColorType>(warning),
		static_cast<ColorType>(success),
		static_cast<ColorType>(note),
		static_cast<ColorType>(flair),
		static_cast<ColorType>(header),
		static_cast<ColorType>(build),
		static_cast<ColorType>(assembly));
}

/*****************************************************************************/
bool ColorTheme::operator==(const ColorTheme& rhs) const
{
	return info == rhs.info
		&& error == rhs.error
		&& warning == rhs.warning
		&& success == rhs.success
		&& note == rhs.note
		&& flair == rhs.flair
		&& info == rhs.info
		&& header == rhs.header
		&& build == rhs.build
		&& assembly == rhs.assembly
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
	if (inValue.empty())
	{
		m_preset.clear();
		return;
	}

	if (!List::contains(state.presetNames, inValue))
	{
		if (inValue.find_first_not_of("01234567890ABCDEFabcdef") == std::string::npos)
			m_preset = inValue;
		else
			m_preset = state.presetNames.at(0);
	}
	else
	{
		m_preset = inValue;
	}

	makePreset(m_preset);
}

bool ColorTheme::isPreset() const noexcept
{
	return !m_preset.empty();
}

/*****************************************************************************/
void ColorTheme::makePreset(const std::string& inValue)
{
	/*
		Known "bad" colors:
			Color::Black & variations - Won't show in Command Prompt default - same color as bg
			Color::Magenta & variations - Won't show in Powershell default - same color as bg

		... avoid them in theme presets
	*/

	reset = Color::Reset;

	// default
	std::size_t i = 0;
	if (String::equals(state.presetNames.at(i), inValue))
	{
		info = Color::Reset;
		error = Color::BrightRedBold;
		warning = Color::BrightYellowBold;
		success = Color::BrightGreenBold;
		note = Color::BrightCyanBold;
		flair = Color::BrightBlack;
		header = Color::BrightYellowBold;
		build = Color::BrightBlue;
		assembly = Color::BrightMagenta;
	}
	// none
	else if (String::equals(state.presetNames.at(++i), inValue))
	{
		reset = Color::None;
		//
		info = Color::None;
		error = Color::None;
		warning = Color::None;
		success = Color::None;
		note = Color::None;
		flair = Color::None;
		header = Color::None;
		build = Color::None;
		assembly = Color::None;
	}
	// palapa
	else if (String::equals(state.presetNames.at(++i), inValue))
	{
		info = Color::Reset;
		error = Color::RedBold;
		warning = Color::YellowBold;
		success = Color::BrightMagentaBold;
		note = Color::BlueBold;
		flair = Color::BrightBlueDim;
		header = Color::BrightGreenBold;
		build = Color::BrightCyan;
		assembly = Color::Yellow;
	}
	// highrise
	else if (String::equals(state.presetNames.at(++i), inValue))
	{
		info = Color::Reset;
		error = Color::BrightRedBold;
		warning = Color::YellowBold;
		success = Color::BrightYellowBold;
		note = Color::BrightMagentaBold;
		flair = Color::BrightBlack;
		header = Color::BrightBlueBold;
		build = Color::BrightMagenta;
		assembly = Color::BrightCyan;
	}
	// teahouse
	else if (String::equals(state.presetNames.at(++i), inValue))
	{
		info = Color::Reset;
		error = Color::RedBold;
		warning = Color::YellowBold;
		success = Color::GreenBold;
		note = Color::BrightYellowBold;
		flair = Color::YellowDim;
		header = Color::YellowBold;
		build = Color::BrightGreen;
		assembly = Color::Cyan;
	}
	// skilodge
	else if (String::equals(state.presetNames.at(++i), inValue))
	{
		info = Color::Reset;
		error = Color::BrightRedBold;
		warning = Color::YellowBold;
		success = Color::BrightYellowBold;
		note = Color::BrightBlackBold;
		flair = Color::BrightBlueDim;
		header = Color::BrightWhiteBold;
		build = Color::BrightCyan;
		assembly = Color::BrightBlue;
	}
	// temple
	else if (String::equals(state.presetNames.at(++i), inValue))
	{
		info = Color::Reset;
		error = Color::BrightRedBold;
		warning = Color::BrightYellowBold;
		success = Color::YellowBold;
		note = Color::BrightMagentaBold;
		flair = Color::BrightMagentaDim;
		header = Color::BrightCyanBold;
		build = Color::BrightRed;
		assembly = Color::BrightYellow;
	}
	// bungalow
	else if (String::equals(state.presetNames.at(++i), inValue))
	{
		info = Color::Reset;
		error = Color::BrightRedBold;
		warning = Color::BrightYellowBold;
		success = Color::BrightGreenBold;
		note = Color::BrightCyanBold;
		flair = Color::BrightBlack;
		header = Color::YellowBold;
		build = Color::Cyan;
		assembly = Color::BrightRed;
	}
	// cottage
	else if (String::equals(state.presetNames.at(++i), inValue))
	{
		info = Color::Reset;
		error = Color::BrightMagentaBold;
		warning = Color::BrightYellowBold;
		success = Color::BrightGreenBold;
		note = Color::BrightBlueBold;
		flair = Color::BrightBlack;
		header = Color::BrightRedBold;
		build = Color::BrightYellow;
		assembly = Color::BrightWhite;
	}
	// monastery
	else if (String::equals(state.presetNames.at(++i), inValue))
	{
		info = Color::Reset;
		error = Color::BrightRedBold;
		warning = Color::YellowBold;
		success = Color::BrightGreenBold;
		note = Color::BrightBlueBold;
		flair = Color::CyanDim;
		header = Color::BrightYellowBold;
		build = Color::BrightWhite;
		assembly = Color::BrightBlack;
	}
	// longhouse
	else if (String::equals(state.presetNames.at(++i), inValue))
	{
		info = Color::Reset;
		error = Color::RedBold;
		warning = Color::YellowBold;
		success = Color::BrightGreenBold;
		note = Color::BrightBlueBold;
		flair = Color::BrightBlack;
		header = Color::WhiteBold;
		build = Color::BrightBlack;
		assembly = Color::Green;
	}
	// greenhouse
	else if (String::equals(state.presetNames.at(++i), inValue))
	{
		info = Color::Reset;
		error = Color::BrightRedBold;
		warning = Color::BrightYellowBold;
		success = Color::BrightGreenBold;
		note = Color::BrightBlueBold;
		flair = Color::BrightBlack;
		header = Color::BrightMagentaBold;
		build = Color::BrightGreen;
		assembly = Color::Green;
	}
	// observatory
	else if (String::equals(state.presetNames.at(++i), inValue))
	{
		info = Color::Reset;
		error = Color::BrightRedBold;
		warning = Color::BrightYellowBold;
		success = Color::BrightWhiteBold;
		note = Color::BrightCyanBold;
		flair = Color::BrightBlack;
		header = Color::BrightMagentaBold;
		build = Color::BrightBlue;
		assembly = Color::Blue;
	}
	// yurt
	else if (String::equals(state.presetNames.at(++i), inValue))
	{
		info = Color::Reset;
		error = Color::BrightRedBold;
		warning = Color::BrightYellowBold;
		success = Color::BrightGreenBold;
		note = Color::BrightBlueBold;
		flair = Color::BrightBlack;
		header = Color::BrightWhiteBold;
		build = Color::Yellow;
		assembly = Color::BrightMagenta;
	}
	else if (inValue.find_first_not_of("01234567890ABCDEFabcdef") == std::string::npos)
	{
		*this = ColorTheme::fromHex(inValue);
		m_preset = "custom";
	}
}

/*****************************************************************************/

#define GET_COLORS(inKey)                       \
	if (String::equals("info", inKey))          \
		return &info;                           \
	else if (String::equals("error", inKey))    \
		return &error;                          \
	else if (String::equals("warning", inKey))  \
		return &warning;                        \
	else if (String::equals("success", inKey))  \
		return &success;                        \
	else if (String::equals("note", inKey))     \
		return &note;                           \
	else if (String::equals("flair", inKey))    \
		return &flair;                          \
	else if (String::equals("header", inKey))   \
		return &header;                         \
	else if (String::equals("build", inKey))    \
		return &build;                          \
	else if (String::equals("assembly", inKey)) \
		return &assembly;                       \
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
