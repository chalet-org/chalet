/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_COLOR_THEME_HPP
#define CHALET_COLOR_THEME_HPP

#include "Terminal/Color.hpp"

namespace chalet
{
struct ColorTheme
{
	Color info = Color::Reset;
	Color error = Color::Reset;
	Color warning = Color::Reset;
	Color success = Color::Reset;
	Color note = Color::Reset;
	//
	Color flair = Color::Reset;
	Color header = Color::Reset;
	Color build = Color::Reset;
	Color assembly = Color::Reset;

	static std::string getStringFromColor(const Color inColor);
	static StringList getJsonColors();
	static StringList getKeys();
	static const std::string& defaultPresetName();
	static bool isValidPreset(const std::string& inPresetName);
	static std::vector<ColorTheme> getAllThemes();

	ColorTheme() = default;
	explicit ColorTheme(const std::string& inPresetName);

	bool set(const std::string& inKey, const std::string& inValue);
	std::string getAsString(const std::string& inKey) const;
	std::string asString() const;

	bool operator==(const ColorTheme& rhs) const;
	bool operator!=(const ColorTheme& rhs) const;

	const std::string& preset() const noexcept;
	void setPreset(const std::string& inValue);
	bool isPreset() const noexcept;

private:
	static Color getColorFromKey(const std::string& inString);

	void makePreset(const std::string& inValue);

	Color* getColorFromString(const std::string& inKey);
	const Color* getColorFromString(const std::string& inKey) const;

	std::string m_preset;
};
}

#endif // CHALET_COLOR_THEME_HPP
