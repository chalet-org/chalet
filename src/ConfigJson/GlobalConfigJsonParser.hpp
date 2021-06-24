/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_GLOBAL_CONFIG_JSON_PARSER_HPP
#define CHALET_GLOBAL_CONFIG_JSON_PARSER_HPP

#include "ConfigJson/IConfigJsonParser.hpp"
#include "Libraries/Json.hpp"

namespace chalet
{
struct CommandLineInputs;
struct JsonFile;
struct StatePrototype;
struct GlobalConfigState;

struct GlobalConfigJsonParser final : IConfigJsonParser
{
	explicit GlobalConfigJsonParser(const CommandLineInputs& inInputs, StatePrototype& inPrototype, JsonFile& inJsonFile);

	bool serialize(GlobalConfigState& outState);

private:
	bool makeCache(GlobalConfigState& outState);

	bool serializeFromJsonRoot(Json& inJson, GlobalConfigState& outState);
	bool parseSettings(const Json& inNode, GlobalConfigState& outState);
	bool parseToolchains(const Json& inNode, GlobalConfigState& outState);
	bool parseAncillaryTools(const Json& inNode, GlobalConfigState& outState);
	bool parseApplePlatformSdks(const Json& inNode, GlobalConfigState& outState);

	const CommandLineInputs& m_inputs;
	StatePrototype& m_prototype;
	JsonFile& m_jsonFile;
};
}

#endif // CHALET_GLOBAL_CONFIG_JSON_PARSER_HPP
