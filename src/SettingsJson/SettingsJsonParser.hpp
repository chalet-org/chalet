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
struct CentralState;
struct IntermediateSettingsState;

struct SettingsJsonParser
{
	explicit SettingsJsonParser(CommandLineInputs& inInputs, CentralState& inCentralState, JsonFile& inJsonFile);

	bool serialize(const IntermediateSettingsState& inState);

private:
	bool validatePaths(const bool inWithError);
	bool makeSettingsJson(const IntermediateSettingsState& inState);
	bool serializeFromJsonRoot(Json& inJson);

	bool parseSettings(Json& inNode);

	bool parseTools(Json& inNode);
#if defined(CHALET_MACOS)
	bool detectAppleSdks(const bool inForce = false);
	bool parseAppleSdks(Json& inNode);
#endif

	CommandLineInputs& m_inputs;
	CentralState& m_centralState;
	JsonFile& m_jsonFile;
};
}

#endif // CHALET_SETTINGS_JSON_PARSER_HPP
