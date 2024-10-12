/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "SettingsJson/ThemeSettingsJsonParser.hpp"

#include "Core/CommandLineInputs.hpp"
#include "System/Files.hpp"
#include "Terminal/Output.hpp"
#include "Json/JsonFile.hpp"
#include "Json/JsonKeys.hpp"

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

	auto readFromSettings = [this](const std::string& inFile, ColorTheme& outTheme, const bool inGlobal) -> void {
		if (!Files::pathExists(inFile))
			return;

		JsonFile jsonFile;
		if (!jsonFile.load(inFile))
			Diagnostic::clearErrors();

		UNUSED(serializeFromJsonRoot(jsonFile.root, outTheme, inGlobal));
	};

	// Keys that aren't valid will get ingored,
	//   so we don't need to validate much other than the json itself
	//
	readFromSettings(globalSettings, theme, true);
	readFromSettings(localSettings, theme, false);

	if (!m_updateTheme)
	{
		theme.setPreset(ColorTheme::getDefaultPresetName());
	}

	Output::setTheme(theme);

	return true;
}

/*****************************************************************************/
bool ThemeSettingsJsonParser::serializeFromJsonRoot(const Json& inJson, ColorTheme& outTheme, const bool inGlobal)
{
	if (inJson.is_object() && inJson.contains(Keys::Theme))
	{
		const auto& themeJson = inJson.at(Keys::Theme);
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
				outTheme.setPreset(ColorTheme::getDefaultPresetName());
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
