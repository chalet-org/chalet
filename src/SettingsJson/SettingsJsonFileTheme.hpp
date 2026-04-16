/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include "Libraries/Json.hpp"
#include "Settings/SettingsType.hpp"
#include "Json/IJsonFileParser.hpp"

namespace chalet
{
struct CommandLineInputs;
struct ColorTheme;

struct SettingsJsonFileTheme final : public IJsonFileParser
{
	static bool parse(const CommandLineInputs& inInputs);

private:
	explicit SettingsJsonFileTheme(const CommandLineInputs& inInputs);

	virtual bool deserialize() final;

	bool readFromSettings(const std::string& inFile, ColorTheme& outTheme, const SettingsType inType);
	bool serializeFromJsonRoot(const Json& inJson, ColorTheme& outTheme, const SettingsType inSettingsType);

	const CommandLineInputs& m_inputs;

	bool m_updateTheme = false;
};
}
