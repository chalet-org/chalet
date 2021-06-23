/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_CACHE_JSON_PARSER_HPP
#define CHALET_CACHE_JSON_PARSER_HPP

#include "Compile/ToolchainPreference.hpp"
#include "Libraries/Json.hpp"

namespace chalet
{
struct CommandLineInputs;
struct JsonFile;
struct StatePrototype;
struct GlobalConfigState;

struct CacheJsonParser
{
	explicit CacheJsonParser(const CommandLineInputs& inInputs, StatePrototype& inPrototype, JsonFile& inJsonFile);

	bool serialize(const GlobalConfigState& inState);

private:
	bool validatePaths();
	bool makeCache(const GlobalConfigState& inState);
	bool serializeFromJsonRoot(Json& inJson);

	bool parseSettings(const Json& inNode);

	bool parseTools(Json& inNode);
#if defined(CHALET_MACOS)
	bool parseAppleSdks(Json& inNode);
#endif

	const CommandLineInputs& m_inputs;
	StatePrototype& m_prototype;
	JsonFile& m_jsonFile;

	const std::string kKeyAncillaryTools = "ancillaryTools";
	const std::string kKeySettings = "settings";
	const std::string kKeyToolchains = "toolchains";
	const std::string kKeyApplePlatformSdks = "applePlatformSdks";
	const std::string kKeyDumpAssembly = "dumpAssembly";
	const std::string kKeyMaxJobs = "maxJobs";
	const std::string kKeyShowCommands = "showCommands";
	const std::string kKeyLastToolchain = "toolchain";
	const std::string kKeyMacosSigningIdentity = "macosSigningIdentity";
	// const std::string kKeyTargetArchitecture = "targetArchitecture";
	const std::string kKeyWorkingDirectory = "workingDirectory";
	const std::string kKeyExternalDependencies = "externalDependencies";
	const std::string kKeyData = "data";

	// should match executables
	const std::string kKeyBash = "bash";
	const std::string kKeyBrew = "brew";
	const std::string kKeyCodesign = "codesign";
	const std::string kKeyCommandPrompt = "command_prompt";
	const std::string kKeyGit = "git";
	const std::string kKeyGprof = "gprof";
	const std::string kKeyHdiutil = "hdiutil";
	const std::string kKeyInstallNameTool = "install_name_tool";
	const std::string kKeyInstruments = "instruments";
	const std::string kKeyLdd = "ldd";
	const std::string kKeyLipo = "lipo";
	const std::string kKeyLua = "lua";
	const std::string kKeyOsascript = "osascript";
	const std::string kKeyOtool = "otool";
	const std::string kKeyPerl = "perl";
	const std::string kKeyPlutil = "plutil";
	const std::string kKeyPowershell = "powershell";
	const std::string kKeyPython = "python";
	const std::string kKeyPython3 = "python3";
	const std::string kKeyRuby = "ruby";
	const std::string kKeySample = "sample";
	const std::string kKeySips = "sips";
	const std::string kKeyTiffutil = "tiffutil";
	const std::string kKeyXcodebuild = "xcodebuild";
	const std::string kKeyXcodegen = "xcodegen";
	const std::string kKeyXcrun = "xcrun";
};
}

#endif // CHALET_CACHE_JSON_PARSER_HPP
