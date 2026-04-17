/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include "Compile/ToolchainPreference.hpp"
#include "Libraries/Json.hpp"
#include "Json/IJsonFileParser.hpp"
#include "Json/JsonFile.hpp"

namespace chalet
{
class BuildState;

struct SettingsJsonFileToolchain final : public IJsonFileParser
{
	static bool read(BuildState& inState);
	static bool validatePaths(BuildState& inState);
	static bool isCurrentToolchainStillValid(BuildState& inState);

private:
	explicit SettingsJsonFileToolchain(BuildState& inState);

	virtual bool deserialize() final;

	bool validatePaths();
	bool isCurrentToolchainStillValid();

	Json& getToolchainNode(Json& inToolchainsNode);
	bool deserializeFromToolchainNode(Json& inNode, const bool inIsCustomToolchain);
	bool makeToolchain(Json& toolchain, const ToolchainPreference& preference, const bool inIsCustomToolchain);

	BuildState& m_state;
	JsonFile& m_jsonFile;

	// bool m_isCustomToolchain = false;
};
}
