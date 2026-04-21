/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include "Libraries/Json.hpp"
#include "Settings/SettingsType.hpp"
#include "Json/IJsonFileReader.hpp"

namespace chalet
{
struct CommandLineInputs;
struct ColorTheme;

struct SettingsJsonFileTheme final : public IJsonFileReader
{
	static bool read(const CommandLineInputs& inInputs);

private:
	explicit SettingsJsonFileTheme(const CommandLineInputs& inInputs);

	virtual bool readFrom(JsonFile& inJsonFile) final;

	bool readThemeIfExists(JsonFile& inJsonFile, const std::string& inFile, ColorTheme& outTheme, const SettingsType inType);
	bool readThemeFromJson(const Json& inJson, ColorTheme& outTheme, const SettingsType inSettingsType);

	const CommandLineInputs& m_inputs;
};
}
