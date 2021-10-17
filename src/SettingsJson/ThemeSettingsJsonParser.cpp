/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "SettingsJson/ThemeSettingsJsonParser.hpp"

#include "Core/CommandLineInputs.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Output.hpp"
#include "Json/JsonFile.hpp"

namespace chalet
{
/*****************************************************************************/
ThemeSettingsJsonParser::ThemeSettingsJsonParser(const CommandLineInputs& inInputs) :
	m_inputs(inInputs)
{
}

/*****************************************************************************/
bool ThemeSettingsJsonParser::serialize()
{
	const auto globalSettings = m_inputs.getGlobalSettingsFilePath();
	const auto& localSettings = m_inputs.settingsFile();

	m_updateTheme = false;
	ColorTheme theme = Output::theme();

	// Keys that aren't valid will get ingored,
	//   so we don't need to validate much other than the json itself
	//
	if (Commands::pathExists(globalSettings))
	{
		JsonFile jsonFile;
		if (!jsonFile.load(globalSettings))
			return false;

		if (!serializeFromJsonRoot(jsonFile.json, theme, true))
		{
			Diagnostic::error("There was an error parsing {}", jsonFile.filename());
			return false;
		}
	}

	if (Commands::pathExists(localSettings))
	{
		JsonFile jsonFile;
		if (!jsonFile.load(localSettings))
			return false;

		if (!serializeFromJsonRoot(jsonFile.json, theme))
		{
			Diagnostic::error("There was an error parsing {}", jsonFile.filename());
			return false;
		}
	}

	if (!m_updateTheme)
	{
		theme.setPreset(ColorTheme::defaultPresetName());
	}

#if 0
	theme.setPreset(ColorTheme::lastPresetName());
#endif
	Output::setTheme(theme);

	return true;
}

/*****************************************************************************/
bool ThemeSettingsJsonParser::serializeFromJsonRoot(const Json& inJson, ColorTheme& outTheme, const bool inGlobal)
{
	const std::string kKeyTheme{ "theme" };

	if (inJson.is_object() && inJson.contains(kKeyTheme))
	{
		const auto& themeJson = inJson.at(kKeyTheme);
		if (themeJson.is_string())
		{
			auto preset = themeJson.get<std::string>();
			outTheme.setPreset(preset); // if invalid, goes to default theme
			m_updateTheme = true;
		}
		else if (themeJson.is_object())
		{
			if (themeJson.empty() && inGlobal)
			{
				outTheme.setPreset(ColorTheme::defaultPresetName());
				outTheme.setPreset(std::string());
			}

			for (auto& [key, value] : themeJson.items())
			{
				if (value.is_string())
				{
					auto str = value.get<std::string>();
					outTheme.set(key, str);
				}
			}
			m_updateTheme = true;
		}
	}

	return true;
}
}
