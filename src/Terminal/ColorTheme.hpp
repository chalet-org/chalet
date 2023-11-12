/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include "Terminal/Color.hpp"

namespace chalet
{
struct ColorTheme
{
	Color reset = Color::None;
	Color info = Color::None;
	Color error = Color::None;
	Color warning = Color::None;
	Color success = Color::None;
	Color note = Color::None;
	//
	Color flair = Color::None;
	Color header = Color::None;
	Color build = Color::None;
	Color assembly = Color::None;

	static ColorTheme fromHex(const std::string& inHex, const std::string name = std::string());
	static std::string getStringFromColor(const Color inColor);
	static StringList getJsonColors();
	static StringList getKeys();
	static std::string getDefaultPresetName();
	static StringList getPresetNames();
	static bool isValidPreset(const std::string& inPresetName);
	static std::vector<ColorTheme> getAllThemes();

	ColorTheme() = default;
	explicit ColorTheme(const std::string& inPresetName);

	bool set(const std::string& inKey, const std::string& inValue);
	std::string getAsString(const std::string& inKey) const;
	std::string asString() const;
	std::string asHexString() const;

	bool operator==(const ColorTheme& rhs) const;
	bool operator!=(const ColorTheme& rhs) const;

	const std::string& preset() const noexcept;
	void setPreset(const std::string& inValue);
	bool isPreset() const noexcept;

private:
	static Color getColorFromDigit(char value);
	static Color getColorFromDigit(const char inValue, const i32 inOffset);
	static Color getColorFromKey(const std::string& inString);

	void makePreset(std::string inValue);

	Color* getColorFromString(const std::string& inKey);
	const Color* getColorFromString(const std::string& inKey) const;

	std::string m_preset;
};
}
