/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_CACHE_TOOLCHAIN_PARSER_HPP
#define CHALET_CACHE_TOOLCHAIN_PARSER_HPP

#include "Compile/ToolchainPreference.hpp"
#include "Libraries/Json.hpp"

namespace chalet
{
struct CommandLineInputs;
class BuildState;
struct JsonFile;

struct CacheToolchainParser
{
	explicit CacheToolchainParser(const CommandLineInputs& inInputs, BuildState& inState, JsonFile& inJsonFile);

	bool serialize();
	bool serialize(Json& inNode);

private:
	bool validatePaths();
#if defined(CHALET_WIN32)
	bool createMsvcEnvironment(const Json& inNode);
#endif
	bool makeToolchain(Json& toolchains, const ToolchainPreference& toolchain);
	bool parseToolchain(Json& inNode);

	bool parseArchitecture(std::string& outString) const;

	const CommandLineInputs& m_inputs;
	BuildState& m_state;
	JsonFile& m_jsonFile;

	const std::string kKeyWorkingDirectory = "workingDirectory";

	const std::string kKeyStrategy = "strategy";
	const std::string kKeyArchiver = "archiver";
	const std::string kKeyCpp = "C++";
	const std::string kKeyCc = "C";
	const std::string kKeyLinker = "linker";
	const std::string kKeyWindowsResource = "windowsResource";

	//
	const std::string kKeyCmake = "cmake";
	const std::string kKeyMake = "make";
	const std::string kKeyNinja = "ninja";
	const std::string kKeyObjdump = "objdump";

	std::string m_make;
};
}

#endif // CHALET_CACHE_TOOLCHAIN_PARSER_HPP
