/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_CACHE_JSON_PARSER_HPP
#define CHALET_CACHE_JSON_PARSER_HPP

#include "State/BuildState.hpp"
#include "State/CommandLineInputs.hpp"
#include "Json/JsonParser.hpp"

namespace chalet
{
struct CacheJsonParser : public JsonParser
{
	explicit CacheJsonParser(const CommandLineInputs& inInputs, BuildState& inState);

	virtual bool serialize() final;

private:
	bool createMsvcEnvironment();
	bool validatePaths();
	bool setDefaultBuildStrategy();
	bool makeCache();
	bool serializeFromJsonRoot(const Json& inJson);

	bool parseRoot(const Json& inNode);

	bool parseTools(const Json& inNode);
	bool parseCompilers(const Json& inNode);

	const CommandLineInputs& m_inputs;
	BuildState& m_state;
	const std::string& m_filename;

	const std::string kKeyTools = "tools";
	const std::string kKeyCompilerTools = "compilerTools";
	const std::string kKeyStrategy = "strategy";
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
	const std::string kKeyLua = "lua";
	const std::string kKeyMacosSdk = "macosSdk";
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
