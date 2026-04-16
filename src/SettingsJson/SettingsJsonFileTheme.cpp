/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "SettingsJson/SettingsJsonFileTheme.hpp"

#include "Core/CommandLineInputs.hpp"
#include "System/Files.hpp"
#include "Terminal/Output.hpp"
#include "Json/JsonFile.hpp"
#include "Json/JsonKeys.hpp"

namespace chalet
{
/*****************************************************************************/
bool SettingsJsonFileTheme::parse(const CommandLineInputs& inInputs)
{
	SettingsJsonFileTheme SettingsJsonFileTheme(inInputs);
	return SettingsJsonFileTheme.deserialize();
}

/*****************************************************************************/
SettingsJsonFileTheme::SettingsJsonFileTheme(const CommandLineInputs& inInputs) :
	m_inputs(inInputs)
{
}

/*****************************************************************************/
bool SettingsJsonFileTheme::deserialize()
{
	m_updateTheme = false;
	ColorTheme theme = Output::theme();

	// Keys that aren't valid will get ingored,
	//   so we don't need to validate much other than the json itself
	//
	const auto globalSettings = m_inputs.getGlobalSettingsFilePath();
	readFromSettings(globalSettings, theme, SettingsType::Global);

	const auto& localSettings = m_inputs.settingsFile();
	readFromSettings(localSettings, theme, SettingsType::Local);

	if (!m_updateTheme)
	{
		theme.setPreset(ColorTheme::getDefaultPresetName());
	}

	Output::setTheme(theme);

	return true;
}

/*****************************************************************************/
bool SettingsJsonFileTheme::readFromSettings(const std::string& inFile, ColorTheme& outTheme, const SettingsType inType)
{
	if (!Files::pathExists(inFile))
		return false;

	JsonFile jsonFile;
	if (!jsonFile.load(inFile))
		Diagnostic::clearErrors();

	return serializeFromJsonRoot(jsonFile.root, outTheme, inType);
}

/*****************************************************************************/
bool SettingsJsonFileTheme::serializeFromJsonRoot(const Json& inJson, ColorTheme& outTheme, const SettingsType inSettingsType)
{
	if (inJson.is_object() && inJson.contains(Keys::Theme))
	{
		const auto& themeJson = inJson[Keys::Theme];
		if (themeJson.is_string())
		{
			auto preset = themeJson.get<std::string>();
			outTheme.setPreset(preset); // if invalid, goes to default theme
			m_updateTheme = true;
		}
		else if (themeJson.is_object())
		{
			if (themeJson.empty() && inSettingsType == SettingsType::Global)
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
