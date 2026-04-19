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
bool SettingsJsonFileTheme::read(const CommandLineInputs& inInputs)
{
	JsonFile jsonFile;
	SettingsJsonFileTheme SettingsJsonFileTheme(inInputs);
	return SettingsJsonFileTheme.readFrom(jsonFile);
}

/*****************************************************************************/
SettingsJsonFileTheme::SettingsJsonFileTheme(const CommandLineInputs& inInputs) :
	m_inputs(inInputs)
{}

/*****************************************************************************/
bool SettingsJsonFileTheme::readFrom(JsonFile& inJsonFile)
{
	bool updateTheme = false;
	ColorTheme theme = Output::theme();

	// Keys that aren't valid will get ingored,
	//   so we don't need to validate much other than the json itself
	//
	const auto globalSettings = m_inputs.getGlobalSettingsFilePath();
	updateTheme |= readThemeIfExists(inJsonFile, globalSettings, theme, SettingsType::Global);

	const auto& localSettings = m_inputs.settingsFile();
	updateTheme |= readThemeIfExists(inJsonFile, localSettings, theme, SettingsType::Local);

	if (!updateTheme)
	{
		theme.setPreset(ColorTheme::getDefaultPresetName());
	}

	Output::setTheme(theme);

	return true;
}

/*****************************************************************************/
bool SettingsJsonFileTheme::readThemeIfExists(JsonFile& inJsonFile, const std::string& inFile, ColorTheme& outTheme, const SettingsType inType)
{
	if (!Files::pathExists(inFile))
		return false;

	if (!inJsonFile.load(inFile))
		Diagnostic::clearErrors();

	bool result = readThemeFromJson(inJsonFile.root, outTheme, inType);

	inJsonFile.root.clear();
	return result;
}

/*****************************************************************************/
bool SettingsJsonFileTheme::readThemeFromJson(const Json& inJson, ColorTheme& outTheme, const SettingsType inSettingsType)
{
	if (inJson.is_object() && inJson.contains(Keys::Theme))
	{
		const auto& themeJson = inJson[Keys::Theme];
		if (themeJson.is_string())
		{
			auto preset = themeJson.get<std::string>();
			outTheme.setPreset(preset); // if invalid, goes to default theme

			return true;
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

			return true;
		}
	}
	return false;
}
}
