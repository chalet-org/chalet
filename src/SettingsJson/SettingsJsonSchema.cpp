/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "SettingsJson/SettingsJsonSchema.hpp"

#include "State/CompilerTools.hpp"
#include "System/SuppressIntellisense.hpp"
#include "Terminal/ColorTheme.hpp"
#include "Utility/String.hpp"
#include "Json/JsonComments.hpp"
#include "Json/JsonKeys.hpp"

namespace chalet
{
namespace
{
enum class Defs : u16
{
	/* Tools */
	Bash,
	CommandPrompt,
	CodeSign,
	Git,
	HdiUtil,
	InstallNameTool,
	Instruments,
	Ldd,
	OsaScript,
	Otool,
	PlUtil,
	Powershell,
	Sample,
	Sips,
	Tar,
	TiffUtil,
	XcodeBuild,
	XcRun,
	Zip,

	/* Toolchains */
	Version,
	ToolchainBuildStrategy,
	ToolchainBuildPathStyle,
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
	OnlyRequired,
	MaxJobs,
	ShowCommands,
	Benchmark,
	KeepGoing,
	LaunchProfiler,
	LastBuildConfiguration,
	LastToolchain,
	LastArchitecture,
	SigningIdentity,
	OsTargetName,
	OsTargetVersion,
	InputFile,
	SettingsFile,
	EnvFile,
	RootDir,
	OutputDir,
	ExternalDir,
	DistributionDir,
	LastTarget,
	RunArguments,
	Theme,
	LastUpdateCheck,

	/* Theme */
	ThemeColor
};
}
/*****************************************************************************/
Json SettingsJsonSchema::get()
{
	Json ret;
	ret["$schema"] = "http://json-schema.org/draft-07/schema";
	ret["type"] = "object";
	ret["additionalProperties"] = false;
	/*ret[SKeys::Required] = {
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

	defs[Defs::Tar] = R"json({
		"type": "string",
		"description": "The executable path to tar.",
		"default": "/usr/bin/tar"
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

	defs[Defs::XcRun] = R"json({
		"type": "string",
		"description": "The executable path to Apple's xcrun command-line utility. (MacOS)",
		"default": "/usr/bin/xcrun"
	})json"_ojson;

	defs[Defs::Zip] = R"json({
		"type": "string",
		"description": "The executable path to zip.",
		"default": "/usr/bin/zip"
	})json"_ojson;

	//
	// toolchain
	//

	defs[Defs::Version] = R"json({
		"type": "string",
		"description": "A version string to identify the toolchain. If MSVC, this must be the full version string of the Visual Studio Installation. (vswhere's installationVersion string)"
	})json"_ojson;

	defs[Defs::ToolchainBuildStrategy] = R"json({
		"type": "string",
		"description": "The strategy to use during the build.",
		"enum": [],
		"default": "makefile"
	})json"_ojson;
	defs[Defs::ToolchainBuildStrategy][SKeys::Enum] = CompilerTools::getToolchainStrategiesForSchema();

	defs[Defs::ToolchainBuildPathStyle] = R"json({
		"type": "string",
		"description": "The build path style, with the configuration appended by an underscore. Examples:\nconfiguration: build/Debug\narchitecture: build/x86_64_Debug\ntarget-triple: build/x64-linux-gnu_Debug\ntoolchain-name: build/my-cool-toolchain_name_Debug",
		"enum": [],
		"default": "target-triple"
	})json"_ojson;
	defs[Defs::ToolchainBuildPathStyle][SKeys::Enum] = CompilerTools::getToolchainBuildPathStyles();

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
		"description": "true to use create an asm dump of each file in the build, false otherwise (default).",
		"default": false
	})json"_ojson;

	defs[Defs::GenerateCompileCommands] = R"json({
		"type": "boolean",
		"description": "true to generate a compile_commands.json file for Clang tooling use, false otherwise (default).",
		"default": false
	})json"_ojson;

	defs[Defs::OnlyRequired] = R"json({
		"type": "boolean",
		"description": "true to only build targets required by the target given at the command line (if not all), false otherwise (default).",
		"default": false
	})json"_ojson;

	defs[Defs::MaxJobs] = R"json({
		"type": "integer",
		"description": "The number of jobs to run during compilation (default: the number of cpu cores).",
		"minimum": 1
	})json"_ojson;

	defs[Defs::ShowCommands] = R"json({
		"type": "boolean",
		"description": "true to show the commands run during the build, false to just show the source file (default).",
		"default": false
	})json"_ojson;

	defs[Defs::Benchmark] = R"json({
		"type": "boolean",
		"description": "true to show all build times (total build time, build targets, other steps) (default), false to hide them.",
		"default": true
	})json"_ojson;

	defs[Defs::KeepGoing] = R"json({
		"type": "boolean",
		"description": "true to continue as much of the build as possible if there's a build error, false to halt on error (default).",
		"default": false
	})json"_ojson;

	defs[Defs::LaunchProfiler] = R"json({
		"type": "boolean",
		"description": "If running profile targets, true to launch the preferred profiler afterwards (default), false to just generate the output files.",
		"default": true
	})json"_ojson;

	defs[Defs::LastBuildConfiguration] = R"json({
		"type": "string",
		"description": "The build configuration to use for building, if not the previous one."
	})json"_ojson;

	defs[Defs::LastToolchain] = R"json({
		"type": "string",
		"description": "The toolchain id to use for building, if not the previous one."
	})json"_ojson;

	defs[Defs::LastArchitecture] = R"json({
		"type": "string",
		"description": "The architecture id to use for building, if not the previous one."
	})json"_ojson;

	defs[Defs::SigningIdentity] = R"json({
		"type": "string",
		"description": "The code-signing identity to use when bundling the application distribution."
	})json"_ojson;

	defs[Defs::OsTargetName] = R"json({
		"type": "string",
		"description": "The name of the operating system to target the build for. On macOS, this corresponds to the lower-case identifier of the Apple SDK (see 'appleSdks')"
	})json"_ojson;

	defs[Defs::OsTargetVersion] = R"json({
		"type": "string",
		"description": "The version of the operating system to target the build for."
	})json"_ojson;

	defs[Defs::InputFile] = R"json({
		"type": "string",
		"description": "An input build file to use.",
		"default": "chalet.json"
	})json"_ojson;

	defs[Defs::EnvFile] = R"json({
		"type": "string",
		"description": "A file to load environment variables from.",
		"default": ".env"
	})json"_ojson;

	defs[Defs::RootDir] = R"json({
		"type": "string",
		"description": "The root directory to run the build from."
	})json"_ojson;

	defs[Defs::OutputDir] = R"json({
		"type": "string",
		"description": "The output directory of the build.",
		"default": "build"
	})json"_ojson;

	defs[Defs::ExternalDir] = R"json({
		"type": "string",
		"description": "The directory to install external dependencies into prior to the rest of the build's run.",
		"default": "chalet_external"
	})json"_ojson;

	defs[Defs::DistributionDir] = R"json({
		"type": "string",
		"description": "The root directory of all distribution bundles."
	})json"_ojson;

	defs[Defs::LastTarget] = R"json({
		"type": "string",
		"description": "The last build target used (or ran), or 'all' if one was not specified."
	})json"_ojson;

	defs[Defs::RunArguments] = R"json({
		"type": "object",
		"description": "An object of key/values where the key is the run target name, and the value is the run arguments that were used last."
	})json"_ojson;

	//
	// other
	//

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
				"type": "string",
				"minLength": 1,
				"pattern": "^[0-9a-fA-F]{1,8}$",
				"default": "default"
			},
			{
				"type": "object",
				"additionalProperties": false
			}
		]
	})json"_ojson;
	defs[Defs::Theme][SKeys::OneOf][0][SKeys::Enum] = ColorTheme::getPresetNames();
	defs[Defs::Theme][SKeys::OneOf][2][SKeys::Properties] = Json::object();

	Json themeRef = Json::object();
	themeRef["$ref"] = std::string("#/definitions/theme-color");
	for (const auto& key : ColorTheme::getKeys())
	{
		defs[Defs::Theme][SKeys::OneOf][2][SKeys::Properties][key] = themeRef;
	}

	defs[Defs::ThemeColor] = R"json({
		"type": "string",
		"description": "An ANSI color to apply."
	})json"_ojson;
	defs[Defs::ThemeColor][SKeys::Enum] = ColorTheme::getJsonColors();

	defs[Defs::LastUpdateCheck] = R"json({
		"type": "number",
		"description": "The time of the last Chalet update check."
	})json"_ojson;

	//

	auto toolchain = R"json({
		"type": "object",
		"description": "A list of compilers and tools needing for the build itself.",
		"additionalProperties": false
	})json"_ojson;
	toolchain[SKeys::Properties] = Json::object();
	toolchain[SKeys::Properties][Keys::ToolchainArchiver] = defs[Defs::Archiver];
	toolchain[SKeys::Properties][Keys::ToolchainBuildPathStyle] = defs[Defs::ToolchainBuildPathStyle];
	toolchain[SKeys::Properties][Keys::ToolchainCMake] = defs[Defs::CMake];
	toolchain[SKeys::Properties][Keys::ToolchainCompilerC] = defs[Defs::CompilerC];
	toolchain[SKeys::Properties][Keys::ToolchainCompilerCpp] = defs[Defs::CompilerCpp];
	toolchain[SKeys::Properties][Keys::ToolchainCompilerWindowsResource] = defs[Defs::CompilerWindowsResource];
	toolchain[SKeys::Properties][Keys::ToolchainDisassembler] = defs[Defs::Disassembler];
	toolchain[SKeys::Properties][Keys::ToolchainLinker] = defs[Defs::Linker];
	toolchain[SKeys::Properties][Keys::ToolchainMake] = defs[Defs::Make];
	toolchain[SKeys::Properties][Keys::ToolchainNinja] = defs[Defs::Ninja];
	toolchain[SKeys::Properties][Keys::ToolchainProfiler] = defs[Defs::Profiler];
	toolchain[SKeys::Properties][Keys::ToolchainBuildStrategy] = defs[Defs::ToolchainBuildStrategy];
	toolchain[SKeys::Properties][Keys::ToolchainVersion] = defs[Defs::Version];

	Json toolchainRef = Json::object();
	toolchainRef["$ref"] = std::string("#/definitions/toolchain");

	//
	ret[SKeys::Definitions] = Json::object();
	ret[SKeys::Definitions]["theme-color"] = defs[Defs::ThemeColor];
	ret[SKeys::Definitions]["toolchain"] = toolchain;

	//
	ret[SKeys::Properties] = Json::object();

	ret[SKeys::Properties][Keys::Options] = R"json({
		"type": "object",
		"description": "A list of settings related to the build."
	})json"_ojson;
	ret[SKeys::Properties][Keys::Options][SKeys::Properties] = Json::object();
	ret[SKeys::Properties][Keys::Options][SKeys::Properties][Keys::OptionsArchitecture] = defs[Defs::LastArchitecture];
	ret[SKeys::Properties][Keys::Options][SKeys::Properties][Keys::OptionsBenchmark] = defs[Defs::Benchmark];
	ret[SKeys::Properties][Keys::Options][SKeys::Properties][Keys::OptionsBuildConfiguration] = defs[Defs::LastBuildConfiguration];
	ret[SKeys::Properties][Keys::Options][SKeys::Properties][Keys::OptionsDistributionDirectory] = defs[Defs::DistributionDir];
	ret[SKeys::Properties][Keys::Options][SKeys::Properties][Keys::OptionsDumpAssembly] = defs[Defs::DumpAssembly];
	ret[SKeys::Properties][Keys::Options][SKeys::Properties][Keys::OptionsEnvFile] = defs[Defs::EnvFile];
	ret[SKeys::Properties][Keys::Options][SKeys::Properties][Keys::OptionsExternalDirectory] = defs[Defs::ExternalDir];
	ret[SKeys::Properties][Keys::Options][SKeys::Properties][Keys::OptionsGenerateCompileCommands] = defs[Defs::GenerateCompileCommands];
	ret[SKeys::Properties][Keys::Options][SKeys::Properties][Keys::OptionsOnlyRequired] = defs[Defs::OnlyRequired];
	ret[SKeys::Properties][Keys::Options][SKeys::Properties][Keys::OptionsInputFile] = defs[Defs::InputFile];
	ret[SKeys::Properties][Keys::Options][SKeys::Properties][Keys::OptionsKeepGoing] = defs[Defs::KeepGoing];
	ret[SKeys::Properties][Keys::Options][SKeys::Properties][Keys::OptionsLaunchProfiler] = defs[Defs::LaunchProfiler];
	ret[SKeys::Properties][Keys::Options][SKeys::Properties][Keys::OptionsMaxJobs] = defs[Defs::MaxJobs];
	ret[SKeys::Properties][Keys::Options][SKeys::Properties][Keys::OptionsOutputDirectory] = defs[Defs::OutputDir];
	ret[SKeys::Properties][Keys::Options][SKeys::Properties][Keys::OptionsRootDirectory] = defs[Defs::RootDir];
	ret[SKeys::Properties][Keys::Options][SKeys::Properties][Keys::OptionsLastTarget] = defs[Defs::LastTarget];
	ret[SKeys::Properties][Keys::Options][SKeys::Properties][Keys::OptionsRunArguments] = defs[Defs::RunArguments];
	ret[SKeys::Properties][Keys::Options][SKeys::Properties][Keys::OptionsShowCommands] = defs[Defs::ShowCommands];
	ret[SKeys::Properties][Keys::Options][SKeys::Properties][Keys::OptionsSigningIdentity] = defs[Defs::SigningIdentity];
	ret[SKeys::Properties][Keys::Options][SKeys::Properties][Keys::OptionsOsTargetName] = defs[Defs::OsTargetName];
	ret[SKeys::Properties][Keys::Options][SKeys::Properties][Keys::OptionsOsTargetVersion] = defs[Defs::OsTargetVersion];
	ret[SKeys::Properties][Keys::Options][SKeys::Properties][Keys::OptionsToolchain] = defs[Defs::LastToolchain];

	ret[SKeys::Properties][Keys::Tools] = R"json({
		"type": "object",
		"description": "A list of additional tools for the platform."
	})json"_ojson;
	ret[SKeys::Properties][Keys::Tools][SKeys::Properties] = Json::object();
	ret[SKeys::Properties][Keys::Tools][SKeys::Properties][Keys::ToolsBash] = defs[Defs::Bash];
	ret[SKeys::Properties][Keys::Tools][SKeys::Properties][Keys::ToolsCommandPrompt] = defs[Defs::CommandPrompt];
	ret[SKeys::Properties][Keys::Tools][SKeys::Properties][Keys::ToolsCodesign] = defs[Defs::CodeSign];
	ret[SKeys::Properties][Keys::Tools][SKeys::Properties][Keys::ToolsGit] = defs[Defs::Git];
	ret[SKeys::Properties][Keys::Tools][SKeys::Properties][Keys::ToolsHdiutil] = defs[Defs::HdiUtil];
	ret[SKeys::Properties][Keys::Tools][SKeys::Properties][Keys::ToolsInstallNameTool] = defs[Defs::InstallNameTool];
	ret[SKeys::Properties][Keys::Tools][SKeys::Properties][Keys::ToolsInstruments] = defs[Defs::Instruments];
	ret[SKeys::Properties][Keys::Tools][SKeys::Properties][Keys::ToolsLdd] = defs[Defs::Ldd];
	ret[SKeys::Properties][Keys::Tools][SKeys::Properties][Keys::ToolsOsascript] = defs[Defs::OsaScript];
	ret[SKeys::Properties][Keys::Tools][SKeys::Properties][Keys::ToolsOtool] = defs[Defs::Otool];
	ret[SKeys::Properties][Keys::Tools][SKeys::Properties][Keys::ToolsPlutil] = defs[Defs::PlUtil];
	ret[SKeys::Properties][Keys::Tools][SKeys::Properties][Keys::ToolsPowershell] = defs[Defs::Powershell];
	ret[SKeys::Properties][Keys::Tools][SKeys::Properties][Keys::ToolsSample] = defs[Defs::Sample];
	ret[SKeys::Properties][Keys::Tools][SKeys::Properties][Keys::ToolsSips] = defs[Defs::Sips];
	ret[SKeys::Properties][Keys::Tools][SKeys::Properties][Keys::ToolsTar] = defs[Defs::Tar];
	ret[SKeys::Properties][Keys::Tools][SKeys::Properties][Keys::ToolsTiffutil] = defs[Defs::TiffUtil];
	ret[SKeys::Properties][Keys::Tools][SKeys::Properties][Keys::ToolsXcodebuild] = defs[Defs::XcodeBuild];
	ret[SKeys::Properties][Keys::Tools][SKeys::Properties][Keys::ToolsXcrun] = defs[Defs::XcRun];
	ret[SKeys::Properties][Keys::Tools][SKeys::Properties][Keys::ToolsZip] = defs[Defs::Zip];

	{
		std::string toolchainNamePattern{ R"(^[\w\-+.]{3,}$)" };
		ret[SKeys::Properties][Keys::Toolchains] = R"json({
			"type": "object",
			"description": "A list of toolchains."
		})json"_ojson;
		ret[SKeys::Properties][Keys::Toolchains][SKeys::PatternProperties][toolchainNamePattern][SKeys::OneOf] = Json::array();
		ret[SKeys::Properties][Keys::Toolchains][SKeys::PatternProperties][toolchainNamePattern][SKeys::OneOf][0] = toolchainRef;

		std::string toolchainArchPattern{ R"(^[\w\-+.]{3,}$)" };
		ret[SKeys::Properties][Keys::Toolchains][SKeys::PatternProperties][toolchainNamePattern][SKeys::OneOf][1] = R"json({
			"type": "object",
			"patternProperties": {},
			"additionalProperties": false
		})json"_ojson;
		ret[SKeys::Properties][Keys::Toolchains][SKeys::PatternProperties][toolchainNamePattern][SKeys::OneOf][1][SKeys::PatternProperties][toolchainArchPattern] = toolchainRef;
		ret[SKeys::Properties][Keys::Toolchains][SKeys::PatternProperties][toolchainNamePattern][SKeys::OneOf][1][SKeys::PatternProperties][toolchainArchPattern][SKeys::Description] = "A list of compilers and tools needing for this toolchain architecture.";
	}

	ret[SKeys::Properties][Keys::AppleSdks] = R"json({
		"type": "object",
		"description": "A list of Apple platform SDK paths. (MacOS)"
	})json"_ojson;

	ret[SKeys::Properties][Keys::Theme] = defs[Defs::Theme];
	ret[SKeys::Properties][Keys::LastUpdateCheck] = defs[Defs::LastUpdateCheck];

	return ret;
}
}
