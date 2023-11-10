/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

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
	bool serializeFromJsonRoot(const Json& inJson, ColorTheme& outTheme, const bool inGlobal = false);

	const CommandLineInputs& m_inputs;

	bool m_updateTheme = false;
};
}
