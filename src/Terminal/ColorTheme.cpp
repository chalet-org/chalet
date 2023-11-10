/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Terminal/ColorTheme.hpp"

#include "Process/Environment.hpp"
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

	const OrderedDictionary<std::string> presets{
		{ "default", "1397d53b" },
		{ "none", "00000000" },
		{ "palapa", "9db27428" },
		{ "highrise", "197b3527" },
		{ "teahouse", "22dac423" },
		{ "skilodge", "9fb93521" },
		{ "temple", "7b532537" },
		{ "bungalow", "12a5d53b" },
		{ "cottage", "153fd739" },
		{ "monastery", "a3f1d529" },
		{ "longhouse", "1e1cd429" },
		{ "greenhouse", "17dcd539" },
		{ "observatory", "1798f53b" },
		{ "yurt", "1f27d539" },
		{ "sealab", "89b8d539" },
		{ "blacklodge", "556f3579" },
		{ "farmhouse", "1bd33729" },
		{ "gallery", "1ff0f539" }
	};
} state;
}

/*****************************************************************************/
Color ColorTheme::getColorFromDigit(char value)
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
Color ColorTheme::getColorFromDigit(const char inValue, const i32 inOffset)
{
	auto color = getColorFromDigit(inValue);
	if (color == Color::Black || color == Color::Reset)
		return color;

	return static_cast<Color>(inOffset + static_cast<i32>(color));
}

/*****************************************************************************/
ColorTheme ColorTheme::fromHex(const std::string& digits, const std::string name)
{
	ColorTheme theme;
	theme.m_preset = name;

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

	if (String::equals("00000000", digits))
	{
		theme.reset = Color::None;
		theme.info = Color::None;
	}

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
std::string ColorTheme::getDefaultPresetName()
{
	return "default";
}

StringList ColorTheme::getPresetNames()
{
	StringList ret;
	for (auto& [preset, _] : state.presets)
		ret.emplace_back(preset);

	return ret;
}

/*****************************************************************************/
bool ColorTheme::isValidPreset(const std::string& inPresetName)
{
	if (inPresetName.empty())
		return false;

	return state.presets.find(inPresetName) != state.presets.end();
}

/*****************************************************************************/
std::vector<ColorTheme> ColorTheme::getAllThemes()
{
	std::vector<ColorTheme> ret;

	for (auto& [preset, _] : state.presets)
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
std::string ColorTheme::asHexString() const
{
	auto colorToHexValue = [](const Color& inColor) -> char {
		i32 color = static_cast<i32>(inColor) % 100;
		for (char i = 0; i < 16; ++i)
		{
			auto col = getColorFromDigit(i);
			if (col == static_cast<Color>(color))
			{
				if (i < 10)
					return i + 48;

				i -= 10;
				return i + 97;
			}
		}
		return '0';
	};

	std::string ret;

	ret += colorToHexValue(this->flair);
	ret += colorToHexValue(this->header);
	ret += colorToHexValue(this->build);
	ret += colorToHexValue(this->assembly);
	ret += colorToHexValue(this->success);
	ret += colorToHexValue(this->error);
	ret += colorToHexValue(this->warning);
	ret += colorToHexValue(this->note);

	return ret;
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

	if (state.presets.find(inValue) == state.presets.end())
	{
		if (inValue.find_first_not_of("01234567890ABCDEFabcdef") == std::string::npos)
			m_preset = inValue;
		else
			m_preset = state.presets.begin()->first;
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
void ColorTheme::makePreset(std::string inValue)
{
	/*
		Known "bad" colors:
			Color::Black & variations - Won't show in Command Prompt default - same color as bg
			Color::Magenta & variations - Won't show in Powershell default - same color as bg

		... avoid them in theme presets

		Also note that Windows Terminal does not differentiate between Normal/Bright bold - only Bright
	*/

	info = Color::Reset;
	reset = Color::Reset;

	if (state.presets.find(inValue) != state.presets.end())
	{
		*this = ColorTheme::fromHex(state.presets.at(inValue), inValue);
	}
	else if (inValue.find_first_not_of("01234567890ABCDEFabcdef") == std::string::npos)
	{
		*this = ColorTheme::fromHex(inValue);
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
