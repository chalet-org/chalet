/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_GLOBAL_CONFIG_JSON_PARSER_HPP
#define CHALET_GLOBAL_CONFIG_JSON_PARSER_HPP

#include "Libraries/Json.hpp"

namespace chalet
{
struct CommandLineInputs;
struct JsonFile;
struct StatePrototype;
struct GlobalConfigState;

struct GlobalConfigJsonParser
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

	const std::string kKeySettings = "settings";
	const std::string kKeyToolchains = "toolchains";
	const std::string kKeyAncillaryTools = "ancillaryTools";
	const std::string kKeyApplePlatformSdks = "applePlatformSdks";

	const std::string kKeyDumpAssembly = "dumpAssembly";
	const std::string kKeyMaxJobs = "maxJobs";
	const std::string kKeyShowCommands = "showCommands";
	const std::string kKeyLastToolchain = "toolchain";
	const std::string kKeyMacosSigningIdentity = "macosSigningIdentity";
};
}

#endif // CHALET_GLOBAL_CONFIG_JSON_PARSER_HPP
