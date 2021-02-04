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
	bool validatePaths();
	bool makeCache();
	bool serializeFromJsonRoot(const Json& inJson);

	bool parseRoot(const Json& inNode);

	bool parseTools(const Json& inNode);
	bool parseCompilers(const Json& inNode);

	const CommandLineInputs& m_inputs;
	BuildState& m_state;
	const std::string& m_filename;

	const std::string kKeyTools = "tools";
	const std::string kKeyCompilers = "compilers";
	const std::string kKeyStrategy = "strategy";
	const std::string kKeyWorkingDirectory = "workingDirectory";
	const std::string kKeyExternalDependencies = "externalDependencies";
	const std::string kKeyData = "data";

	const std::string kKeyCpp = "C++";
	const std::string kKeyCc = "C";
	const std::string kKeyWindowsResource = "windowsResource";

	// should match executables
	const std::string kKeyAr = "ar";
	const std::string kKeyBrew = "brew";
	const std::string kKeyCmake = "cmake";
	const std::string kKeyCodeSign = "codesign";
	const std::string kKeyGit = "git";
	const std::string kKeyHdiUtil = "hdiutil";
	const std::string kKeyInstallNameTool = "install_name_tool";
	const std::string kKeyLdd = "ldd";
	const std::string kKeyMacosSdk = "macosSdk";
	const std::string kKeyMake = "make";
	const std::string kKeyNinja = "ninja";
	const std::string kKeyOtool = "otool";
	const std::string kKeyPlUtil = "plutil";
	const std::string kKeyRanLib = "ranlib";
	const std::string kKeySips = "sips";
	const std::string kKeyStrip = "strip";
	const std::string kKeyTiffUtil = "tiffutil";

	std::string m_make;
};
}

#endif // CHALET_CACHE_JSON_PARSER_HPP
