/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_SETTINGS_JSON_PARSER_HPP
#define CHALET_SETTINGS_JSON_PARSER_HPP

#include "Compile/ToolchainPreference.hpp"
#include "Libraries/Json.hpp"

namespace chalet
{
struct CommandLineInputs;
struct JsonFile;
struct StatePrototype;
struct GlobalSettingsState;

struct SettingsJsonParser
{
	explicit SettingsJsonParser(CommandLineInputs& inInputs, StatePrototype& inPrototype, JsonFile& inJsonFile);

	bool serialize(const GlobalSettingsState& inState);

private:
	bool validatePaths();
	bool makeSettingsJson(const GlobalSettingsState& inState);
	bool serializeFromJsonRoot(Json& inJson);

	bool parseSettings(Json& inNode);

	bool parseTools(Json& inNode);
#if defined(CHALET_MACOS)
	bool parseAppleSdks(Json& inNode);
#endif

	CommandLineInputs& m_inputs;
	StatePrototype& m_prototype;
	JsonFile& m_jsonFile;
};
}

#endif // CHALET_SETTINGS_JSON_PARSER_HPP
