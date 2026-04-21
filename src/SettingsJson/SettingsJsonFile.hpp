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
struct CommandLineInputs;
struct CentralState;
struct IntermediateSettingsState;

struct SettingsJsonFile final : public IJsonFileReader
{
	static bool read(CommandLineInputs& inInputs, CentralState& inCentralState, const IntermediateSettingsState& inFallback);

private:
	explicit SettingsJsonFile(CommandLineInputs& inInputs, CentralState& inCentralState, const IntermediateSettingsState& inFallback);

	virtual bool readFrom(JsonFile& inJsonFile) final;

	bool validateFileContentsAgainstSchema(const JsonFile& inJsonFile);
	bool validatePlatformSDKs(JsonFile& inJsonFile);

	bool readFromSettings(Json& inNode);
	bool readFromTools(Json& inNode);
	void readFromPlatformSDKs(Json& inNode);

#if defined(CHALET_WIN32)
	bool detectPathsToGitAndLLD(Json& inToolsNode);
#elif defined(CHALET_MACOS)
	bool detectPathsToPlatformSDKs(Json& inNode, const bool inForce = false);
#endif

	CommandLineInputs& m_inputs;
	CentralState& m_centralState;

	const IntermediateSettingsState& m_fallback;
};
}
