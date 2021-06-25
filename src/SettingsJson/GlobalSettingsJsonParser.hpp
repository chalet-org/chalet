/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_GLOBAL_SETTINGS_JSON_PARSER_HPP
#define CHALET_GLOBAL_SETTINGS_JSON_PARSER_HPP

#include "Libraries/Json.hpp"
#include "SettingsJson/ISettingsJsonParser.hpp"

namespace chalet
{
struct CommandLineInputs;
struct JsonFile;
struct StatePrototype;
struct GlobalSettingsState;

struct GlobalSettingsJsonParser final : ISettingsJsonParser
{
	explicit GlobalSettingsJsonParser(const CommandLineInputs& inInputs, StatePrototype& inPrototype, JsonFile& inJsonFile);

	bool serialize(GlobalSettingsState& outState);

private:
	bool makeCache(GlobalSettingsState& outState);

	bool serializeFromJsonRoot(Json& inJson, GlobalSettingsState& outState);
	bool parseSettings(const Json& inNode, GlobalSettingsState& outState);
	bool parseToolchains(const Json& inNode, GlobalSettingsState& outState);
	bool parseAncillaryTools(const Json& inNode, GlobalSettingsState& outState);
	bool parseApplePlatformSdks(const Json& inNode, GlobalSettingsState& outState);

	const CommandLineInputs& m_inputs;
	StatePrototype& m_prototype;
	JsonFile& m_jsonFile;
};
}

#endif // CHALET_GLOBAL_SETTINGS_JSON_PARSER_HPP
