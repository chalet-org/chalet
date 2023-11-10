/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

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
	bool makeToolchain(Json& toolchain, const ToolchainPreference& preference);
	bool parseToolchain(Json& inNode);

	BuildState& m_state;
	JsonFile& m_jsonFile;

	bool m_isCustomToolchain = false;
};
}
