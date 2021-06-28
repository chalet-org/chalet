/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "SettingsJson/SchemaSettingsJson.hpp"

#include "Utility/String.hpp"
#include "Utility/SuppressIntellisense.hpp"
#include "Json/JsonComments.hpp"

namespace chalet
{
namespace
{
enum class Defs : ushort
{
	// Tools
	Bash,
	Brew,
	CommandPrompt,
	CodeSign,
	Git,
	GProf,
	HdiUtil,
	InstallNameTool,
	Instruments,
	Ldd,
	Lipo,
	Lua,
	OsaScript,
	Otool,
	Perl,
	PlUtil,
	Powershell,
	Python,
	Python3,
	Ruby,
	Sample,
	Sips,
	TiffUtil,
	XcodeBuild,
	XcodeGen,
	XcRun,
	// Toolchains
	CppCompiler,
	CCompiler,
	WinResourceCompiler,
	Archiver,
	Linker,
	Make,
	CMake,
	Ninja,
	ObjDump,
	ToolchainStrategy,
	// Settings
	DumpAssembly,
	MaxJobs,
	ShowCommands,
	Toolchain,
	SigningIdentity,
};
}
/*****************************************************************************/
Json Schema::getSettingsJson()
{
	Json ret;
	ret["$schema"] = "http://json-schema.org/draft-07/schema";
	ret["type"] = "object";
	ret["additionalProperties"] = false;
	ret["required"] = {
		"settings",
		"tools",
		"toolchains"
	};

	//
	// const auto kDefinitions = "definitions";
	// ret[kDefinitions] = Json::object();
	std::unordered_map<Defs, Json> defs;

	defs[Defs::Bash] = R"json({
		"type": "string",
		"description": "The executable path to GNU bash",
		"default": "/usr/bin/bash"
	})json"_ojson;

	defs[Defs::Brew] = R"json({
		"type": "string",
		"description": "The executable path to brew command-line utility (MacOS)",
		"default": "/usr/local/bin/brew"
	})json"_ojson;

	defs[Defs::CommandPrompt] = R"json({
		"type": "string",
		"description": "The executable path to Command Prompt (Windows)",
		"default": "C:/Windows/System32/cmd.exe"
	})json"_ojson;

	defs[Defs::CodeSign] = R"json({
		"type": "string",
		"description": "The executable path to codesign command-line utility (MacOS)",
		"default": "/usr/bin/codesign"
	})json"_ojson;

	defs[Defs::Git] = R"json({
		"type": "string",
		"description": "The executable path to git.",
		"default": "/usr/bin/git"
	})json"_ojson;

	defs[Defs::GProf] = R"json({
		"type": "string",
		"description": "The executable path to gprof.",
		"default": "/usr/bin/gprof"
	})json"_ojson;

	defs[Defs::HdiUtil] = R"json({
		"type": "string",
		"description": "The executable path to Apple's hdiutil command-line utility (MacOS)",
		"default": "/usr/bin/hdiutil"
	})json"_ojson;

	defs[Defs::InstallNameTool] = R"json({
		"type": "string",
		"description": "The executable path to Apple's install_name_tool command-line utility (MacOS)",
		"default": "/usr/bin/install_name_tool"
	})json"_ojson;

	defs[Defs::Instruments] = R"json({
		"type": "string",
		"description": "The executable path to Apple's instruments command-line utility (MacOS)",
		"default": "/usr/bin/instruments"
	})json"_ojson;

	defs[Defs::Ldd] = R"json({
		"type": "string",
		"description": "The executable path to ldd.",
		"default": "/usr/bin/ldd"
	})json"_ojson;

	defs[Defs::Lipo] = R"json({
		"type": "string",
		"description": "The executable path to Apple's lipo utility (MacOS)",
		"default": "/usr/bin/lipo"
	})json"_ojson;

	defs[Defs::Lua] = R"json({
		"type": "string",
		"description": "The executable path to Lua",
		"default": "/usr/local/bin/lua"
	})json"_ojson;

	defs[Defs::OsaScript] = R"json({
		"type": "string",
		"description": "The executable path to Apple's osascript command-line utility (MacOS)",
		"default": "/usr/bin/osascript"
	})json"_ojson;

	defs[Defs::Otool] = R"json({
		"type": "string",
		"description": "The executable path to Apple's otool command-line utility (MacOS)",
		"default": "/usr/bin/otool"
	})json"_ojson;

	defs[Defs::Perl] = R"json({
		"type": "string",
		"description": "The executable path to Perl",
		"default": "/usr/local/bin/perl"
	})json"_ojson;

	defs[Defs::PlUtil] = R"json({
		"type": "string",
		"description": "The executable path to Apple's plutil command-line utility (MacOS)",
		"default": "/usr/bin/plutil"
	})json"_ojson;

	defs[Defs::Powershell] = R"json({
		"type": "string",
		"description": "The executable path to Powershell (Windows)",
		"default": "C:/Windows/System32/WindowsPowerShell/v1.0/powershell.exe"
	})json"_ojson;

	defs[Defs::Python] = R"json({
		"type": "string",
		"description": "The executable path to Python 2",
		"default": "/usr/bin/python"
	})json"_ojson;

	defs[Defs::Python3] = R"json({
		"type": "string",
		"description": "The executable path to Python 3",
		"default": "/usr/local/bin/python3"
	})json"_ojson;

	defs[Defs::Ruby] = R"json({
		"type": "string",
		"description": "The executable path to ruby.",
		"default": "/usr/bin/ruby"
	})json"_ojson;

	defs[Defs::Sample] = R"json({
		"type": "string",
		"description": "The executable path to Apple's sample command-line utility (MacOS)",
		"default": "/usr/bin/sample"
	})json"_ojson;

	defs[Defs::Sips] = R"json({
		"type": "string",
		"description": "The executable path to Apple's sips command-line utility (MacOS)",
		"default": "/usr/bin/sips"
	})json"_ojson;

	defs[Defs::TiffUtil] = R"json({
		"type": "string",
		"description": "The executable path to tiffutil",
		"default": "/usr/bin/tiffutil"
	})json"_ojson;

	defs[Defs::XcodeBuild] = R"json({
		"type": "string",
		"description": "The executable path to Apple's xcodebuild command-line utility (MacOS)",
		"default": "/usr/bin/xcodebuild"
	})json"_ojson;

	defs[Defs::XcodeGen] = R"json({
		"type": "string",
		"description": "The executable path to the xcodegen Homebrew application (MacOS)",
		"default": "/usr/local/bin/xcodegen"
	})json"_ojson;

	defs[Defs::XcRun] = R"json({
		"type": "string",
		"description": "The executable path to Apple's xcrun command-line utility (MacOS)",
		"default": "/usr/bin/xcrun"
	})json"_ojson;

	// libtool (macOS), ar (Linux / macOS / MinGW), lib.exe (Win)
	defs[Defs::Archiver] = R"json({
		"type": "string",
		"description": "The executable path to the toolchain's static library archive utility",
		"default": "/usr/bin/ar"
	})json"_ojson;

	defs[Defs::CppCompiler] = R"json({
		"type": "string",
		"description": "The executable path to the toolchain's C++ compiler",
		"default": "/usr/bin/c++"
	})json"_ojson;

	defs[Defs::CCompiler] = R"json({
		"type": "string",
		"description": "The executable path to the toolchain's C compiler",
		"default": "/usr/bin/cc"
	})json"_ojson;

	defs[Defs::CMake] = R"json({
		"type": "string",
		"description": "The executable path to CMake",
		"default": "/usr/local/bin/cmake"
	})json"_ojson;

	defs[Defs::Linker] = R"json({
		"type": "string",
		"description": "The executable path to the toolchain's linker",
		"default": "/usr/bin/ld"
	})json"_ojson;

	defs[Defs::Make] = R"json({
		"type": "string",
		"description": "The executable path to GNU make utility.",
		"default": "/usr/bin/make"
	})json"_ojson;

	defs[Defs::Ninja] = R"json({
		"type": "string",
		"description": "The executable path to ninja."
	})json"_ojson;

	defs[Defs::ObjDump] = R"json({
		"type": "string",
		"description": "The executable path to objdump."
	})json"_ojson;

	/*
	// These don't get called directly (yet), but might be useful to look into
	defs[Defs::RanLib] = R"json({
		"type": "string",
		"description": "The executable path to ranlib.",
		"default": "/usr/bin/ranlib"
	})json"_ojson;

	defs[Defs::Strip] = R"json({
		"type": "string",
		"description": "The executable path to strip.",
		"default": "/usr/bin/strip"
	})json"_ojson;
	*/

	defs[Defs::WinResourceCompiler] = R"json({
		"type": "string",
		"description": "The executable path to the resource compiler (Windows)"
	})json"_ojson;

	defs[Defs::ToolchainStrategy] = R"json({
		"type": "string",
		"description": "The build strategy to use.",
		"enum": [
			"makefile",
			"native-experimental",
			"ninja"
		],
		"default": "makefile"
	})json"_ojson;

	defs[Defs::DumpAssembly] = R"json({
		"type": "boolean",
		"description": "true to use include an asm dump of each file in the build, false otherwise.",
		"default": false
	})json"_ojson;

	defs[Defs::MaxJobs] = R"json({
		"type": "integer",
		"description": "The number of threads to run during compilation. If this number exceeds the capabilities of the processor, the processor's max will be used.",
		"minimum": 1
	})json"_ojson;

	defs[Defs::ShowCommands] = R"json({
		"description": "true to show the commands run during the build, false to just show the source file.",
		"type": "boolean",
		"default": false
	})json"_ojson;

	defs[Defs::Toolchain] = R"json({
		"description": "The toolchain id to use for building, if not the previous one.",
		"type": "string"
	})json"_ojson;

	defs[Defs::SigningIdentity] = R"json({
		"description": "The signing identity to use when bundling the macos application bundle.",
		"type": "string"
	})json"_ojson;

	//

	const auto kProperties = "properties";

	auto toolchains = R"json({
		"type": "object",
		"additionalProperties": false,
		"description": "The list of compilers for the platform",
		"required": [
			"archiver",
			"C++",
			"C",
			"cmake",
			"linker",
			"make",
			"objdump",
			"ninja",
			"strategy",
			"windowsResource"
		]
	})json"_ojson;
	toolchains[kProperties]["strategy"] = defs[Defs::ToolchainStrategy];
	toolchains[kProperties]["archiver"] = defs[Defs::Archiver];
	toolchains[kProperties]["C++"] = defs[Defs::CppCompiler];
	toolchains[kProperties]["C"] = defs[Defs::CCompiler];
	toolchains[kProperties]["cmake"] = defs[Defs::CMake];
	toolchains[kProperties]["linker"] = defs[Defs::Linker];
	toolchains[kProperties]["make"] = defs[Defs::Make];
	toolchains[kProperties]["ninja"] = defs[Defs::Ninja];
	toolchains[kProperties]["objdump"] = defs[Defs::ObjDump];
	toolchains[kProperties]["windowsResource"] = defs[Defs::WinResourceCompiler];

	//
	ret[kProperties] = Json::object();

	const auto kTools = "tools";
	ret[kProperties][kTools] = R"json({
		"type": "object",
		"additionalProperties": false,
		"description": "The list of tools for the platform"
	})json"_ojson;
	ret[kProperties][kTools][kProperties]["bash"] = defs[Defs::Bash];
	ret[kProperties][kTools][kProperties]["brew"] = defs[Defs::Brew];
	ret[kProperties][kTools][kProperties]["command_prompt"] = defs[Defs::CommandPrompt];
	ret[kProperties][kTools][kProperties]["codesign"] = defs[Defs::CodeSign];
	ret[kProperties][kTools][kProperties]["git"] = defs[Defs::Git];
	ret[kProperties][kTools][kProperties]["gprof"] = defs[Defs::GProf];
	ret[kProperties][kTools][kProperties]["hdiutil"] = defs[Defs::HdiUtil];
	ret[kProperties][kTools][kProperties]["install_name_tool"] = defs[Defs::InstallNameTool];
	ret[kProperties][kTools][kProperties]["instruments"] = defs[Defs::Instruments];
	ret[kProperties][kTools][kProperties]["ldd"] = defs[Defs::Ldd];
	ret[kProperties][kTools][kProperties]["lipo"] = defs[Defs::Lipo];
	ret[kProperties][kTools][kProperties]["lua"] = defs[Defs::Lua];
	ret[kProperties][kTools][kProperties]["osascript"] = defs[Defs::OsaScript];
	ret[kProperties][kTools][kProperties]["otool"] = defs[Defs::Otool];
	ret[kProperties][kTools][kProperties]["perl"] = defs[Defs::Perl];
	ret[kProperties][kTools][kProperties]["plutil"] = defs[Defs::PlUtil];
	ret[kProperties][kTools][kProperties]["powershell"] = defs[Defs::Powershell];
	ret[kProperties][kTools][kProperties]["python"] = defs[Defs::Python];
	ret[kProperties][kTools][kProperties]["python3"] = defs[Defs::Python3];
	ret[kProperties][kTools][kProperties]["ruby"] = defs[Defs::Ruby];
	ret[kProperties][kTools][kProperties]["sample"] = defs[Defs::Sample];
	ret[kProperties][kTools][kProperties]["sips"] = defs[Defs::Sips];
	ret[kProperties][kTools][kProperties]["tiffutil"] = defs[Defs::TiffUtil];
	ret[kProperties][kTools][kProperties]["xcodebuild"] = defs[Defs::XcodeBuild];
	ret[kProperties][kTools][kProperties]["xcodegen"] = defs[Defs::XcodeGen];
	ret[kProperties][kTools][kProperties]["xcrun"] = defs[Defs::XcRun];

	ret[kProperties]["appleSdks"] = R"json({
		"type": "object",
		"description": "A list of Apple platform SDK paths (MacOS)"
	})json"_ojson;

	ret[kProperties]["workingDirectory"] = R"json({
		"type": "string",
		"description": "The working directory of the workspace"
	})json"_ojson;

	const auto kSettings = "settings";
	ret[kProperties][kSettings] = R"json({
		"type": "object",
		"additionalProperties": false,
		"description": "A list of settings related to the build",
		"required": [
			"dumpAssembly",
			"maxJobs",
			"showCommands",
			"toolchain",
			"signingIdentity"
		]
	})json"_ojson;
	ret[kProperties][kSettings][kProperties]["dumpAssembly"] = defs[Defs::DumpAssembly];
	ret[kProperties][kSettings][kProperties]["maxJobs"] = defs[Defs::MaxJobs];
	ret[kProperties][kSettings][kProperties]["showCommands"] = defs[Defs::ShowCommands];
	ret[kProperties][kSettings][kProperties]["toolchain"] = defs[Defs::Toolchain];
	ret[kProperties][kSettings][kProperties]["signingIdentity"] = defs[Defs::SigningIdentity];

	const auto kToolchains = "toolchains";
	ret[kProperties][kToolchains] = R"json({
		"type": "object",
		"additionalProperties": false,
		"description": "A list of compiler toolchains, mapped to the ids: msvc, llvm, or gcc"
	})json"_ojson;
	ret[kProperties][kToolchains]["patternProperties"][R"(^[\w\-\+\.]{3,}$)"] = toolchains;

	return ret;
}
}
