/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_CACHE_JSON_PARSER_HPP
#define CHALET_CACHE_JSON_PARSER_HPP

#include "Compile/ToolchainPreference.hpp"
#include "Core/CommandLineInputs.hpp"
#include "State/BuildState.hpp"

namespace chalet
{
struct CacheJsonParser
{
	explicit CacheJsonParser(const CommandLineInputs& inInputs, BuildState& inState);

	bool serialize();

private:
	bool createMsvcEnvironment();
	bool validatePaths();
	bool setDefaultBuildStrategy();
	bool makeCache();
	bool makeToolchain(Json& compilerTools, const ToolchainPreference& toolchain);
	bool serializeFromJsonRoot(Json& inJson);

	bool parseRoot(const Json& inNode);
	bool parseSettings(const Json& inNode);

	bool parseTools(Json& inNode);
	bool parseCompilers(Json& inNode);
#if defined(CHALET_MACOS)
	bool parseAppleSdks(Json& inNode);
#endif

	bool parseArchitecture(std::string& outString) const;

	const CommandLineInputs& m_inputs;
	BuildState& m_state;
	const std::string& m_filename;

	const std::string kKeyTools = "tools";
	const std::string kKeySettings = "settings";
	const std::string kKeyCompilerTools = "compilerTools";
	const std::string kKeyApplePlatformSdks = "applePlatformSdks";
	const std::string kKeyDumpAssembly = "dumpAssembly";
	const std::string kKeyMaxJobs = "maxJobs";
	const std::string kKeyStrategy = "strategy";
	const std::string kKeyShowCommands = "showCommands";
	// const std::string kKeyTargetArchitecture = "targetArchitecture";
	const std::string kKeyWorkingDirectory = "workingDirectory";
	const std::string kKeyExternalDependencies = "externalDependencies";
	const std::string kKeyData = "data";

	const std::string kKeyArchiver = "archiver";
	const std::string kKeyCpp = "C++";
	const std::string kKeyCc = "C";
	const std::string kKeyLinker = "linker";
	const std::string kKeyWindowsResource = "windowsResource";

	// should match executables
	const std::string kKeyBash = "bash";
	const std::string kKeyBrew = "brew";
	const std::string kKeyCmake = "cmake";
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
	const std::string kKeyMake = "make";
	const std::string kKeyNinja = "ninja";
	const std::string kKeyObjdump = "objdump";
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

	std::string m_make;

	bool m_changeStrategy = false;
};
}

#endif // CHALET_CACHE_JSON_PARSER_HPP
