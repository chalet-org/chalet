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
struct CommandLineInputs;
struct CentralState;
struct IntermediateSettingsState;

struct SettingsJsonFile final : public IJsonFileParser
{
	static bool read(CommandLineInputs& inInputs, CentralState& inCentralState, const IntermediateSettingsState& inFallback);

private:
	explicit SettingsJsonFile(CommandLineInputs& inInputs, CentralState& inCentralState, const IntermediateSettingsState& inFallback);

	virtual bool deserialize() final;

	bool validatePaths(const bool inWithError);
	bool makeSettingsJson();
	bool serializeFromJsonRoot(Json& inJson);

	bool parseSettings(Json& inNode);

	bool parseTools(Json& inNode);
#if defined(CHALET_WIN32)
	bool detectGitAndLLDPath(Json& inToolsNode);
#elif defined(CHALET_MACOS)
	bool detectAppleSdks(const bool inForce = false);
	bool parseAppleSdks(Json& inNode);
#endif

	CommandLineInputs& m_inputs;
	CentralState& m_centralState;
	JsonFile& m_jsonFile;

	const IntermediateSettingsState& m_fallback;
};
}
