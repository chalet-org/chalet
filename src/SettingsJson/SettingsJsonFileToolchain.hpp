/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include "Compile/ToolchainPreference.hpp"
#include "Libraries/Json.hpp"
#include "Json/IJsonFileReader.hpp"
#include "Json/JsonFile.hpp"

namespace chalet
{
class BuildState;

struct SettingsJsonFileToolchain final : public IJsonFileReader
{
	static bool read(BuildState& inState);
	static bool validatePaths(BuildState& inState);
	static bool isCurrentToolchainStillValid(BuildState& inState);

private:
	explicit SettingsJsonFileToolchain(BuildState& inState);

	virtual bool readFrom(JsonFile& inJsonFile) final;

	bool validatePaths(JsonFile& inJsonFile);
	bool isCurrentToolchainStillValid(JsonFile& inJsonFile);

	Json& getToolchainNode(Json& inToolchainsNode);
	bool readFromToolchainNode(JsonFile& inJsonFile, Json& inNode, const bool inIsCustomToolchain);
	bool detectToolchainPaths(Json& toolchain, const ToolchainPreference& preference, const bool inIsCustomToolchain);

	BuildState& m_state;
};
}
