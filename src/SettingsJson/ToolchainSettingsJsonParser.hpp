/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_TOOLCHAIN_SETTINGS_JSON_PARSER_HPP
#define CHALET_TOOLCHAIN_SETTINGS_JSON_PARSER_HPP

#include "Compile/ToolchainPreference.hpp"
#include "Libraries/Json.hpp"

namespace chalet
{
class BuildState;
struct JsonFile;

struct ToolchainSettingsJsonParser
{
	explicit ToolchainSettingsJsonParser(BuildState& inState, JsonFile& inJsonFile);

	bool serialize();
	bool validatePaths();

private:
	bool serialize(Json& inNode);
	bool makeToolchain(Json& toolchains, const ToolchainPreference& toolchain);
	bool parseToolchain(Json& inNode);

	BuildState& m_state;
	JsonFile& m_jsonFile;
};
}

#endif // CHALET_TOOLCHAIN_SETTINGS_JSON_PARSER_HPP