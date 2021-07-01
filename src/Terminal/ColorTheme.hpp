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
	ColorTheme();

	Color info;
	Color error;
	Color warning;
	Color success;
	//
	Color flair;
	Color header;
	Color build;
	Color alt;
	Color note;

	static bool parseColorFromString(const std::string& inString, Color& outColor);
	static std::string getStringFromColor(const Color inColor);
	static StringList getKeys();

	bool set(const std::string& inKey, const std::string& inValue);
	std::string getAsString(const std::string& inKey);
	std::string asString() const;

	bool operator==(const ColorTheme& rhs) const;
	bool operator!=(const ColorTheme& rhs) const;

private:
	Color* getColorFromString(const std::string& inKey);
};
}

#endif // CHALET_COLOR_THEME_HPP
