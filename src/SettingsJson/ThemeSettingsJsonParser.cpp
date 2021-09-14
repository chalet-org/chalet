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
	const auto& globalSettings = m_inputs.globalSettingsFile();
	const auto& localSettings = m_inputs.settingsFile();

	ColorTheme theme = Output::theme();

	// Keys that aren't valid will get ingored,
	//   so we don't need to validate much other than the json itself
	//
	if (Commands::pathExists(globalSettings))
	{
		JsonFile jsonFile;
		if (!jsonFile.load(globalSettings))
			return false;

		if (!serializeFromJsonRoot(jsonFile.json, theme))
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
	Output::setTheme(theme);

	return true;
}

/*****************************************************************************/
bool ThemeSettingsJsonParser::serializeFromJsonRoot(const Json& inJson, ColorTheme& outTheme)
{
	if (inJson.is_object())
	{
		if (inJson.contains(kKeyTheme))
		{
			const auto& themeJson = inJson.at(kKeyTheme);
			if (themeJson.is_object())
			{
				for (auto& [key, value] : themeJson.items())
				{
					if (value.is_string())
					{
						auto str = value.get<std::string>();
						outTheme.set(key, str);
					}
				}
			}
		}
	}

	return true;
}
}
