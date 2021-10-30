/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "SettingsJson/SchemaSettingsJson.hpp"

#include "Terminal/ColorTheme.hpp"
#include "Utility/String.hpp"
#include "Utility/SuppressIntellisense.hpp"
#include "Json/JsonComments.hpp"

namespace chalet
{
namespace
{
enum class Defs : ushort
{
	/* Tools */
	Bash,
	Brew,
	CommandPrompt,
	CodeSign,
	Git,
	HdiUtil,
	InstallNameTool,
	Instruments,
	Ldd,
	Lua,
	MakeNsis,
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
	// XcodeGen,
	XcRun,

	/* Toolchains */
	Version,
	ToolchainStrategy,
	BuildPathStyle,
	CompilerCpp,
	CompilerC,
	CompilerWindowsResource,
	Archiver,
	Linker,
	Profiler,
	Disassembler,
	Make,
	CMake,
	Ninja,

	/* Settings */
	DumpAssembly,
	GenerateCompileCommands,
	MaxJobs,
	ShowCommands,
	Benchmark,
	LastBuildConfiguration,
	LastToolchain,
	LastArchitecture,
	SigningIdentity,
	InputFile,
	SettingsFile,
	EnvFile,
	RootDir,
	OutputDir,
	ExternalDir,
	DistributionDir,
	Theme,

	/* Theme */
	ThemeColor
};
}
/*****************************************************************************/
Json Schema::getSettingsJson()
{
	const std::string kEnum{ "enum" };
	const std::string kDefinitions{ "definitions" };
	const std::string kProperties{ "properties" };
	const std::string kOneOf{ "oneOf" };
	const std::string kRequired{ "required" };

	Json ret;
	ret["$schema"] = "http://json-schema.org/draft-07/schema";
	ret["type"] = "object";
	ret["additionalProperties"] = false;
	/*ret[kRequired] = {
		"options",
		"tools",
		"toolchains"
	};*/

	std::unordered_map<Defs, Json> defs;

	defs[Defs::Bash] = R"json({
		"type": "string",
		"description": "The executable path to GNU Bourne-Again SHell.",
		"default": "/usr/bin/bash"
	})json"_ojson;

	defs[Defs::Brew] = R"json({
		"type": "string",
		"description": "The executable path to the Homebrew package manager. (MacOS)",
		"default": "/usr/local/bin/brew"
	})json"_ojson;

	defs[Defs::CommandPrompt] = R"json({
		"type": "string",
		"description": "The executable path to Command Prompt. (Windows)",
		"default": "C:/Windows/System32/cmd.exe"
	})json"_ojson;

	defs[Defs::CodeSign] = R"json({
		"type": "string",
		"description": "The executable path to Apple's codesign command-line utility. (MacOS)",
		"default": "/usr/bin/codesign"
	})json"_ojson;

	defs[Defs::Git] = R"json({
		"type": "string",
		"description": "The executable path to Git.",
		"default": "/usr/bin/git"
	})json"_ojson;

	defs[Defs::HdiUtil] = R"json({
		"type": "string",
		"description": "The executable path to Apple's hdiutil command-line utility. (MacOS)",
		"default": "/usr/bin/hdiutil"
	})json"_ojson;

	defs[Defs::InstallNameTool] = R"json({
		"type": "string",
		"description": "The executable path to Apple's install_name_tool command-line utility. (MacOS)",
		"default": "/usr/bin/install_name_tool"
	})json"_ojson;

	defs[Defs::Instruments] = R"json({
		"type": "string",
		"description": "The executable path to Apple's instruments command-line utility. (MacOS)",
		"default": "/usr/bin/instruments"
	})json"_ojson;

	defs[Defs::Ldd] = R"json({
		"type": "string",
		"description": "The executable path to ldd.",
		"default": "/usr/bin/ldd"
	})json"_ojson;

	defs[Defs::Lua] = R"json({
		"type": "string",
		"description": "The executable path to Lua.",
		"default": "/usr/local/bin/lua"
	})json"_ojson;

	defs[Defs::MakeNsis] = R"json({
		"type": "string",
		"description": "The executable path to the Nullsoft Scriptable Install System (NSIS) compiler. (Windows)",
		"default": "C:/Program Files (x86)/NSIS/makensis.exe"
	})json"_ojson;

	defs[Defs::OsaScript] = R"json({
		"type": "string",
		"description": "The executable path to Apple's osascript command-line utility. (MacOS)",
		"default": "/usr/bin/osascript"
	})json"_ojson;

	defs[Defs::Otool] = R"json({
		"type": "string",
		"description": "The executable path to Apple's otool command-line utility. (MacOS)",
		"default": "/usr/bin/otool"
	})json"_ojson;

	defs[Defs::Perl] = R"json({
		"type": "string",
		"description": "The executable path to Perl.",
		"default": "/usr/local/bin/perl"
	})json"_ojson;

	defs[Defs::PlUtil] = R"json({
		"type": "string",
		"description": "The executable path to Apple's plutil command-line utility. (MacOS)",
		"default": "/usr/bin/plutil"
	})json"_ojson;

	defs[Defs::Powershell] = R"json({
		"type": "string",
		"description": "The executable path to Powershell. (Windows)",
		"default": "C:/Windows/System32/WindowsPowerShell/v1.0/powershell.exe"
	})json"_ojson;

	defs[Defs::Python] = R"json({
		"type": "string",
		"description": "The executable path to Python 2.",
		"default": "/usr/bin/python"
	})json"_ojson;

	defs[Defs::Python3] = R"json({
		"type": "string",
		"description": "The executable path to Python 3.",
		"default": "/usr/local/bin/python3"
	})json"_ojson;

	defs[Defs::Ruby] = R"json({
		"type": "string",
		"description": "The executable path to Ruby.",
		"default": "/usr/bin/ruby"
	})json"_ojson;

	defs[Defs::Sample] = R"json({
		"type": "string",
		"description": "The executable path to Apple's sample command-line utility. (MacOS)",
		"default": "/usr/bin/sample"
	})json"_ojson;

	defs[Defs::Sips] = R"json({
		"type": "string",
		"description": "The executable path to Apple's sips command-line utility. (MacOS)",
		"default": "/usr/bin/sips"
	})json"_ojson;

	defs[Defs::TiffUtil] = R"json({
		"type": "string",
		"description": "The executable path to Apple's tiffutil command-line utility. (MacOS)",
		"default": "/usr/bin/tiffutil"
	})json"_ojson;

	defs[Defs::XcodeBuild] = R"json({
		"type": "string",
		"description": "The executable path to Apple's xcodebuild command-line utility. (MacOS)",
		"default": "/usr/bin/xcodebuild"
	})json"_ojson;

	/*defs[Defs::XcodeGen] = R"json({
		"type": "string",
		"description": "The executable path to the xcodegen Homebrew application (MacOS)",
		"default": "/usr/local/bin/xcodegen"
	})json"_ojson;*/

	defs[Defs::XcRun] = R"json({
		"type": "string",
		"description": "The executable path to Apple's xcrun command-line utility. (MacOS)",
		"default": "/usr/bin/xcrun"
	})json"_ojson;

	//
	// toolchain
	//

	defs[Defs::Version] = R"json({
		"type": "string",
		"description": "A version string to identify the toolchain. If MSVC, this must be the full version string of the Visual Studio Installation. (vswhere's installationVersion string)"
	})json"_ojson;

	defs[Defs::ToolchainStrategy] = R"json({
		"type": "string",
		"description": "The strategy to use during the build.",
		"enum": [
			"makefile",
			"native-experimental",
			"ninja"
		],
		"default": "makefile"
	})json"_ojson;

	defs[Defs::BuildPathStyle] = R"json({
		"type": "string",
		"description": "The build path style. Examples:\nconfiguration: build/Debug\narch-configuration: build/x64_Debug\ntarget-triple: build/x64-linux-gnu_Debug\ntoolchain-name: build/my-cool-toolchain_name_Debug",
		"enum": [
			"configuration",
			"arch-configuration",
			"target-triple",
			"toolchain-name"
		],
		"default": "target-triple"
	})json"_ojson;

	// libtool (macOS), ar (Linux / macOS / MinGW), lib.exe (Win)
	defs[Defs::Archiver] = R"json({
		"type": "string",
		"description": "The executable path to the toolchain's static library archive utility - typically ar with GCC, libtool on MacOS, or lib.exe with Visual Studio.",
		"default": "/usr/bin/ar"
	})json"_ojson;

	defs[Defs::CompilerCpp] = R"json({
		"type": "string",
		"description": "The executable path to the toolchain's C++ compiler.",
		"default": "/usr/bin/c++"
	})json"_ojson;

	defs[Defs::CompilerC] = R"json({
		"type": "string",
		"description": "The executable path to the toolchain's C compiler.",
		"default": "/usr/bin/cc"
	})json"_ojson;

	defs[Defs::CMake] = R"json({
		"type": "string",
		"description": "The executable path to CMake.",
		"default": "/usr/local/bin/cmake"
	})json"_ojson;

	defs[Defs::Linker] = R"json({
		"type": "string",
		"description": "The executable path to the toolchain's linker - typically ld with GCC, lld with LLVM, or link.exe with Visual Studio.",
		"default": "/usr/bin/ld"
	})json"_ojson;

	defs[Defs::Profiler] = R"json({
		"type": "string",
		"description": "The executable path to the toochain's command-line profiler (if applicable) - for instance, gprof with GCC.",
		"default": "/usr/bin/gprof"
	})json"_ojson;

	defs[Defs::Make] = R"json({
		"type": "string",
		"description": "The executable path to GNU make, or NMAKE/Qt Jom with Visual Studio.",
		"default": "/usr/bin/make"
	})json"_ojson;

	defs[Defs::Ninja] = R"json({
		"type": "string",
		"description": "The executable path to Ninja."
	})json"_ojson;

	defs[Defs::Disassembler] = R"json({
		"type": "string",
		"description": "The executable path to the toolchain's disassembler (if applicable) - for instance, objdump with GCC, dumpbin with MSVC, and otool with Apple LLVM."
	})json"_ojson;

	defs[Defs::CompilerWindowsResource] = R"json({
		"type": "string",
		"description": "The executable path to the resource compiler. (Windows)"
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

	//
	// settings
	//

	defs[Defs::DumpAssembly] = R"json({
		"type": "boolean",
		"description": "true to use create an asm dump of each file in the build, false otherwise.",
		"default": false
	})json"_ojson;

	defs[Defs::GenerateCompileCommands] = R"json({
		"type": "boolean",
		"description": "true to generate a compile_commands.json file for Clang tooling use, false otherwise.",
		"default": false
	})json"_ojson;

	defs[Defs::MaxJobs] = R"json({
		"type": "integer",
		"description": "The number of jobs to run during compilation.",
		"minimum": 1
	})json"_ojson;

	defs[Defs::ShowCommands] = R"json({
		"description": "true to show the commands run during the build, false to just show the source file.",
		"type": "boolean",
		"default": false
	})json"_ojson;

	defs[Defs::Benchmark] = R"json({
		"description": "true to show all build times (total build time, build targets, other steps), false to hide them.",
		"type": "boolean",
		"default": true
	})json"_ojson;

	defs[Defs::LastBuildConfiguration] = R"json({
		"description": "The build configuration to use for building, if not the previous one.",
		"type": "string"
	})json"_ojson;

	defs[Defs::LastToolchain] = R"json({
		"description": "The toolchain id to use for building, if not the previous one.",
		"type": "string"
	})json"_ojson;

	defs[Defs::LastArchitecture] = R"json({
		"description": "The architecture id to use for building, if not the previous one.",
		"type": "string"
	})json"_ojson;

	defs[Defs::SigningIdentity] = R"json({
		"description": "The signing identity to use when bundling the macos application distribution.",
		"type": "string"
	})json"_ojson;

	defs[Defs::InputFile] = R"json({
		"description": "An input build file to use.",
		"type": "string",
		"default": "chalet.json"
	})json"_ojson;

	defs[Defs::EnvFile] = R"json({
		"description": "A file to load environment variables from.",
		"type": "string",
		"default": ".env"
	})json"_ojson;

	defs[Defs::RootDir] = R"json({
		"description": "The root directory to run the build from.",
		"type": "string"
	})json"_ojson;

	defs[Defs::OutputDir] = R"json({
		"description": "The output directory of the build.",
		"type": "string",
		"default": "build"
	})json"_ojson;

	defs[Defs::ExternalDir] = R"json({
		"type": "string",
		"description": "The directory to install external dependencies into prior to the rest of the build's run.",
		"default": "chalet_external"
	})json"_ojson;

	defs[Defs::DistributionDir] = R"json({
		"description": "The root directory of all distribution bundles.",
		"type": "string"
	})json"_ojson;

	defs[Defs::Theme] = R"json({
		"description": "The color theme preset or colors to give to Chalet",
		"oneOf": [
			{
				"type": "string",
				"minLength": 1,
				"enum": [
					"default"
				],
				"default": "default"
			},
			{
				"type": "object",
				"additionalProperties": false
			}
		]
	})json"_ojson;
	defs[Defs::Theme][kOneOf][0][kEnum] = ColorTheme::presets();
	defs[Defs::Theme][kOneOf][1][kProperties] = Json::object();

	Json themeRef = Json::object();
	themeRef["$ref"] = std::string("#/definitions/theme-color");
	for (const auto& key : ColorTheme::getKeys())
	{
		defs[Defs::Theme][kOneOf][1][kProperties][key] = themeRef;
	}

	defs[Defs::ThemeColor] = R"json({
		"type": "string",
		"description": "An ANSI color to apply."
	})json"_ojson;
	defs[Defs::ThemeColor][kEnum] = ColorTheme::getJsonColors();

	//

	auto toolchains = R"json({
		"type": "object",
		"additionalProperties": false,
		"description": "A list of compilers and tools needing for the build itself."
	})json"_ojson;
	/*toolchains[kRequired] = {
		"archiver",
		"cmake",
		"compilerCpp",
		"compilerC",
		"compilerWindowsResource",
		"linker",
		"profiler",
		"make",
		"disassembler",
		"ninja",
		"strategy",
		"version"
	};*/
	toolchains[kProperties] = Json::object();
	toolchains[kProperties]["version"] = defs[Defs::Version];
	toolchains[kProperties]["strategy"] = defs[Defs::ToolchainStrategy];
	toolchains[kProperties]["buildPathStyle"] = defs[Defs::BuildPathStyle];
	toolchains[kProperties]["archiver"] = defs[Defs::Archiver];
	toolchains[kProperties]["compilerCpp"] = defs[Defs::CompilerCpp];
	toolchains[kProperties]["compilerC"] = defs[Defs::CompilerC];
	toolchains[kProperties]["compilerWindowsResource"] = defs[Defs::CompilerWindowsResource];
	toolchains[kProperties]["cmake"] = defs[Defs::CMake];
	toolchains[kProperties]["linker"] = defs[Defs::Linker];
	toolchains[kProperties]["profiler"] = defs[Defs::Profiler];
	toolchains[kProperties]["make"] = defs[Defs::Make];
	toolchains[kProperties]["ninja"] = defs[Defs::Ninja];
	toolchains[kProperties]["disassembler"] = defs[Defs::Disassembler];

	//
	ret[kDefinitions] = Json::object();
	ret[kDefinitions]["theme-color"] = defs[Defs::ThemeColor];

	ret[kProperties] = Json::object();

	const std::string kKeyTools{ "tools" };
	ret[kProperties][kKeyTools] = R"json({
		"type": "object",
		"additionalProperties": false,
		"description": "A list of additional tools for the platform."
	})json"_ojson;
	ret[kProperties][kKeyTools][kProperties] = Json::object();
	ret[kProperties][kKeyTools][kProperties]["bash"] = defs[Defs::Bash];
	ret[kProperties][kKeyTools][kProperties]["brew"] = defs[Defs::Brew];
	ret[kProperties][kKeyTools][kProperties]["command_prompt"] = defs[Defs::CommandPrompt];
	ret[kProperties][kKeyTools][kProperties]["codesign"] = defs[Defs::CodeSign];
	ret[kProperties][kKeyTools][kProperties]["git"] = defs[Defs::Git];
	ret[kProperties][kKeyTools][kProperties]["hdiutil"] = defs[Defs::HdiUtil];
	ret[kProperties][kKeyTools][kProperties]["install_name_tool"] = defs[Defs::InstallNameTool];
	ret[kProperties][kKeyTools][kProperties]["instruments"] = defs[Defs::Instruments];
	ret[kProperties][kKeyTools][kProperties]["ldd"] = defs[Defs::Ldd];
	ret[kProperties][kKeyTools][kProperties]["lua"] = defs[Defs::Lua];
	ret[kProperties][kKeyTools][kProperties]["makensis"] = defs[Defs::MakeNsis];
	ret[kProperties][kKeyTools][kProperties]["osascript"] = defs[Defs::OsaScript];
	ret[kProperties][kKeyTools][kProperties]["otool"] = defs[Defs::Otool];
	ret[kProperties][kKeyTools][kProperties]["perl"] = defs[Defs::Perl];
	ret[kProperties][kKeyTools][kProperties]["plutil"] = defs[Defs::PlUtil];
	ret[kProperties][kKeyTools][kProperties]["powershell"] = defs[Defs::Powershell];
	ret[kProperties][kKeyTools][kProperties]["python"] = defs[Defs::Python];
	ret[kProperties][kKeyTools][kProperties]["python3"] = defs[Defs::Python3];
	ret[kProperties][kKeyTools][kProperties]["ruby"] = defs[Defs::Ruby];
	ret[kProperties][kKeyTools][kProperties]["sample"] = defs[Defs::Sample];
	ret[kProperties][kKeyTools][kProperties]["sips"] = defs[Defs::Sips];
	ret[kProperties][kKeyTools][kProperties]["tiffutil"] = defs[Defs::TiffUtil];
	ret[kProperties][kKeyTools][kProperties]["xcodebuild"] = defs[Defs::XcodeBuild];
	// ret[kProperties][kKeyTools][kProperties]["xcodegen"] = defs[Defs::XcodeGen];
	ret[kProperties][kKeyTools][kProperties]["xcrun"] = defs[Defs::XcRun];

	ret[kProperties]["appleSdks"] = R"json({
		"type": "object",
		"description": "A list of Apple platform SDK paths. (MacOS)"
	})json"_ojson;

	const std::string kKeyOptions{ "options" };
	ret[kProperties][kKeyOptions] = R"json({
		"type": "object",
		"additionalProperties": false,
		"description": "A list of settings related to the build."
	})json"_ojson;
	/*ret[kProperties][kKeyOptions][kRequired] = {
		"dumpAssembly",
		"maxJobs",
		"showCommands",
		"benchmark",
		"configuration",
		"toolchain",
		"architecture",
		"signingIdentity",
		"inputFile",
		"envFile",
		"rootDir",
		"outputDir",
		"distributionDir",
	};*/
	ret[kProperties][kKeyOptions][kProperties] = Json::object();
	ret[kProperties][kKeyOptions][kProperties]["dumpAssembly"] = defs[Defs::DumpAssembly];
	ret[kProperties][kKeyOptions][kProperties]["generateCompileCommands"] = defs[Defs::GenerateCompileCommands];
	ret[kProperties][kKeyOptions][kProperties]["maxJobs"] = defs[Defs::MaxJobs];
	ret[kProperties][kKeyOptions][kProperties]["showCommands"] = defs[Defs::ShowCommands];
	ret[kProperties][kKeyOptions][kProperties]["benchmark"] = defs[Defs::Benchmark];
	ret[kProperties][kKeyOptions][kProperties]["configuration"] = defs[Defs::LastBuildConfiguration];
	ret[kProperties][kKeyOptions][kProperties]["toolchain"] = defs[Defs::LastToolchain];
	ret[kProperties][kKeyOptions][kProperties]["architecture"] = defs[Defs::LastArchitecture];
	ret[kProperties][kKeyOptions][kProperties]["signingIdentity"] = defs[Defs::SigningIdentity];

	ret[kProperties][kKeyOptions][kProperties]["inputFile"] = defs[Defs::InputFile];
	ret[kProperties][kKeyOptions][kProperties]["envFile"] = defs[Defs::EnvFile];
	ret[kProperties][kKeyOptions][kProperties]["rootDir"] = defs[Defs::RootDir];
	ret[kProperties][kKeyOptions][kProperties]["outputDir"] = defs[Defs::OutputDir];
	ret[kProperties][kKeyOptions][kProperties]["externalDir"] = defs[Defs::ExternalDir];
	ret[kProperties][kKeyOptions][kProperties]["distributionDir"] = defs[Defs::DistributionDir];

	const std::string kKeyTheme{ "theme" };
	ret[kProperties][kKeyTheme] = defs[Defs::Theme];

	const std::string kKeyToolchains{ "toolchains" };
	ret[kProperties][kKeyToolchains] = R"json({
		"type": "object",
		"additionalProperties": false,
		"description": "A list of toolchains."
	})json"_ojson;
	ret[kProperties][kKeyToolchains]["patternProperties"][R"(^[\w\-\+\.]{3,}$)"] = toolchains;

	return ret;
}
}
