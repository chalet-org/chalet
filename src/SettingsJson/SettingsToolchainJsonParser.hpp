/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_SETTINGS_TOOLCHAIN_JSON_PARSER_HPP
#define CHALET_SETTINGS_TOOLCHAIN_JSON_PARSER_HPP

#include "Compile/ToolchainPreference.hpp"
#include "Libraries/Json.hpp"
#include "SettingsJson/ISettingsJsonParser.hpp"

namespace chalet
{
struct CommandLineInputs;
class BuildState;
struct JsonFile;

struct SettingsToolchainJsonParser
{
	explicit SettingsToolchainJsonParser(const CommandLineInputs& inInputs, BuildState& inState, JsonFile& inJsonFile);

	bool serialize();
	bool serialize(Json& inNode);

private:
	bool validatePaths();
	bool makeToolchain(Json& toolchains, const ToolchainPreference& toolchain);
	bool parseToolchain(Json& inNode);

	const CommandLineInputs& m_inputs;
	BuildState& m_state;
	JsonFile& m_jsonFile;

	const std::string kKeyStrategy = "strategy";
	const std::string kKeyArchiver = "archiver";
	const std::string kKeyCpp = "C++";
	const std::string kKeyCc = "C";
	const std::string kKeyLinker = "linker";
	const std::string kKeyProfiler = "profiler";
	const std::string kKeyWindowsResource = "windowsResource";

	//
	const std::string kKeyCmake = "cmake";
	const std::string kKeyMake = "make";
	const std::string kKeyNinja = "ninja";
	const std::string kKeyObjdump = "objdump";
};
}

#endif // CHALET_SETTINGS_TOOLCHAIN_JSON_PARSER_HPP
