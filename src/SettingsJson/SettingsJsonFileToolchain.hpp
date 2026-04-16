/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include "Compile/ToolchainPreference.hpp"
#include "Libraries/Json.hpp"
#include "Json/IJsonFileParser.hpp"

namespace chalet
{
class BuildState;
struct JsonFile;

struct SettingsJsonFileToolchain final : public IJsonFileParser
{
	static bool parse(BuildState& inState);
	static bool validatePaths(BuildState& inState);
	static bool validatePathsWithoutFullParseAndEraseToolchainOnFailure(BuildState& inState);

private:
	explicit SettingsJsonFileToolchain(BuildState& inState);

	virtual bool deserialize() final;

	bool validatePaths();
	bool validatePathsWithoutFullParseAndEraseToolchainOnFailure();

	Json& getToolchainNode(Json& inToolchainsNode);
	bool deserializeFromToolchainNode(Json& inNode, const bool inIsCustomToolchain);
	bool makeToolchain(Json& toolchain, const ToolchainPreference& preference, const bool inIsCustomToolchain);

	BuildState& m_state;
	JsonFile& m_jsonFile;

	// bool m_isCustomToolchain = false;
};
}
