/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_THEME_SETTINGS_JSON_PARSER_HPP
#define CHALET_THEME_SETTINGS_JSON_PARSER_HPP

#include "Libraries/Json.hpp"

namespace chalet
{
struct CommandLineInputs;
struct ColorTheme;

struct ThemeSettingsJsonParser
{
	explicit ThemeSettingsJsonParser(const CommandLineInputs& inInputs);

	bool serialize();

private:
	bool serializeFromJsonRoot(const Json& inJson, ColorTheme& outTheme);

	const CommandLineInputs& m_inputs;
};
}

#endif // CHALET_THEME_JSON_PARSER_HPP
