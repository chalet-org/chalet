/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_GLOBAL_SETTINGS_JSON_PARSER_HPP
#define CHALET_GLOBAL_SETTINGS_JSON_PARSER_HPP

#include "Libraries/Json.hpp"

namespace chalet
{
struct JsonFile;
struct CentralState;
struct IntermediateSettingsState;

struct GlobalSettingsJsonParser
{
	explicit GlobalSettingsJsonParser(CentralState& inCentralState, JsonFile& inJsonFile);

	bool serialize(IntermediateSettingsState& outState);

private:
	bool makeCache(const IntermediateSettingsState& inState);
	void initializeTheme();

	bool serializeFromJsonRoot(Json& inJson, IntermediateSettingsState& outState);
	bool parseSettings(const Json& inNode, IntermediateSettingsState& outState);
	bool parseToolchains(const Json& inNode, IntermediateSettingsState& outState);
	bool parseAncillaryTools(const Json& inNode, IntermediateSettingsState& outState);
#if defined(CHALET_MACOS)
	bool parseApplePlatformSdks(const Json& inNode, IntermediateSettingsState& outState);
#endif
	bool parseLastUpdate(Json& outNode);

	CentralState& m_centralState;
	JsonFile& m_jsonFile;
};
}

#endif // CHALET_GLOBAL_SETTINGS_JSON_PARSER_HPP
