/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "ConfigJson/SchemaConfigJson.hpp"

#include "Utility/String.hpp"
#include "Utility/SuppressIntellisense.hpp"
#include "Json/JsonComments.hpp"

namespace chalet
{
/*****************************************************************************/
Json Schema::getConfigJson()
{
	// Note: By parsing json from a string instead of _ojson literal, we can use ordered_json

	Json ret;
	ret["$schema"] = "http://json-schema.org/draft-07/schema";
	ret["type"] = "object";
	ret["additionalProperties"] = false;
	ret["required"] = {
		"settings",
		"ancillaryTools",
		"toolchains"
	};

	//
	const auto kDefinitions = "definitions";
	ret[kDefinitions] = Json::object();

	ret[kDefinitions]["ancillaryTools-bash"] = R"json({
		"type": "string",
		"description": "The executable path to GNU bash",
		"default": "/usr/bin/bash"
	})json"_ojson;

	ret[kDefinitions]["ancillaryTools-brew"] = R"json({
		"type": "string",
		"description": "The executable path to brew command-line utility (MacOS)",
		"default": "/usr/local/bin/brew"
	})json"_ojson;

	ret[kDefinitions]["ancillaryTools-command_prompt"] = R"json({
		"type": "string",
		"description": "The executable path to Command Prompt (Windows)",
		"default": "C:/Windows/System32/cmd.exe"
	})json"_ojson;

	ret[kDefinitions]["ancillaryTools-codesign"] = R"json({
		"type": "string",
		"description": "The executable path to codesign command-line utility (MacOS)",
		"default": "/usr/bin/codesign"
	})json"_ojson;

	ret[kDefinitions]["ancillaryTools-git"] = R"json({
		"type": "string",
		"description": "The executable path to git.",
		"default": "/usr/bin/git"
	})json"_ojson;

	ret[kDefinitions]["ancillaryTools-gprof"] = R"json({
		"type": "string",
		"description": "The executable path to gprof.",
		"default": "/usr/bin/gprof"
	})json"_ojson;

	ret[kDefinitions]["ancillaryTools-hdiutil"] = R"json({
		"type": "string",
		"description": "The executable path to Apple's hdiutil command-line utility (MacOS)",
		"default": "/usr/bin/hdiutil"
	})json"_ojson;

	ret[kDefinitions]["ancillaryTools-install_name_tool"] = R"json({
		"type": "string",
		"description": "The executable path to Apple's install_name_tool command-line utility (MacOS)",
		"default": "/usr/bin/install_name_tool"
	})json"_ojson;

	ret[kDefinitions]["ancillaryTools-instruments"] = R"json({
		"type": "string",
		"description": "The executable path to Apple's instruments command-line utility (MacOS)",
		"default": "/usr/bin/instruments"
	})json"_ojson;

	ret[kDefinitions]["ancillaryTools-ldd"] = R"json({
		"type": "string",
		"description": "The executable path to ldd.",
		"default": "/usr/bin/ldd"
	})json"_ojson;

	ret[kDefinitions]["ancillaryTools-lipo"] = R"json({
		"type": "string",
		"description": "The executable path to Apple's lipo utility (MacOS)",
		"default": "/usr/bin/lipo"
	})json"_ojson;

	ret[kDefinitions]["ancillaryTools-lua"] = R"json({
		"type": "string",
		"description": "The executable path to Lua",
		"default": "/usr/local/bin/lua"
	})json"_ojson;

	ret[kDefinitions]["ancillaryTools-osascript"] = R"json({
		"type": "string",
		"description": "The executable path to Apple's osascript command-line utility (MacOS)",
		"default": "/usr/bin/osascript"
	})json"_ojson;

	ret[kDefinitions]["ancillaryTools-otool"] = R"json({
		"type": "string",
		"description": "The executable path to Apple's otool command-line utility (MacOS)",
		"default": "/usr/bin/otool"
	})json"_ojson;

	ret[kDefinitions]["ancillaryTools-perl"] = R"json({
		"type": "string",
		"description": "The executable path to Perl",
		"default": "/usr/local/bin/perl"
	})json"_ojson;

	ret[kDefinitions]["ancillaryTools-plutil"] = R"json({
		"type": "string",
		"description": "The executable path to Apple's plutil command-line utility (MacOS)",
		"default": "/usr/bin/plutil"
	})json"_ojson;

	ret[kDefinitions]["ancillaryTools-powershell"] = R"json({
		"type": "string",
		"description": "The executable path to Powershell (Windows)",
		"default": "C:/Windows/System32/WindowsPowerShell/v1.0/powershell.exe"
	})json"_ojson;

	ret[kDefinitions]["ancillaryTools-python"] = R"json({
		"type": "string",
		"description": "The executable path to Python 2",
		"default": "/usr/bin/python"
	})json"_ojson;

	ret[kDefinitions]["ancillaryTools-python3"] = R"json({
		"type": "string",
		"description": "The executable path to Python 3",
		"default": "/usr/local/bin/python3"
	})json"_ojson;

	ret[kDefinitions]["ancillaryTools-ruby"] = R"json({
		"type": "string",
		"description": "The executable path to ruby.",
		"default": "/usr/bin/ruby"
	})json"_ojson;

	ret[kDefinitions]["ancillaryTools-sample"] = R"json({
		"type": "string",
		"description": "The executable path to Apple's sample command-line utility (MacOS)",
		"default": "/usr/bin/sample"
	})json"_ojson;

	ret[kDefinitions]["ancillaryTools-sips"] = R"json({
		"type": "string",
		"description": "The executable path to Apple's sips command-line utility (MacOS)",
		"default": "/usr/bin/sips"
	})json"_ojson;

	ret[kDefinitions]["ancillaryTools-tiffutil"] = R"json({
		"type": "string",
		"description": "The executable path to tiffutil",
		"default": "/usr/bin/tiffutil"
	})json"_ojson;

	ret[kDefinitions]["ancillaryTools-xcodebuild"] = R"json({
		"type": "string",
		"description": "The executable path to Apple's xcodebuild command-line utility (MacOS)",
		"default": "/usr/bin/xcodebuild"
	})json"_ojson;

	ret[kDefinitions]["ancillaryTools-xcodegen"] = R"json({
		"type": "string",
		"description": "The executable path to the xcodegen Homebrew application (MacOS)",
		"default": "/usr/local/bin/xcodegen"
	})json"_ojson;

	ret[kDefinitions]["ancillaryTools-xcrun"] = R"json({
		"type": "string",
		"description": "The executable path to Apple's xcrun command-line utility (MacOS)",
		"default": "/usr/bin/xcrun"
	})json"_ojson;

	// libtool (macOS), ar (Linux / macOS / MinGW), lib.exe (Win)
	ret[kDefinitions]["toolchains-archiver"] = R"json({
		"type": "string",
		"description": "The executable path to the toolchain's static library archive utility",
		"default": "/usr/bin/ar"
	})json"_ojson;

	ret[kDefinitions]["toolchains-cpp"] = R"json({
		"type": "string",
		"description": "The executable path to the toolchain's C++ compiler",
		"default": "/usr/bin/c++"
	})json"_ojson;

	ret[kDefinitions]["toolchains-c"] = R"json({
		"type": "string",
		"description": "The executable path to the toolchain's C compiler",
		"default": "/usr/bin/cc"
	})json"_ojson;

	ret[kDefinitions]["toolchains-cmake"] = R"json({
		"type": "string",
		"description": "The executable path to CMake",
		"default": "/usr/local/bin/cmake"
	})json"_ojson;

	ret[kDefinitions]["toolchains-linker"] = R"json({
		"type": "string",
		"description": "The executable path to the toolchain's linker",
		"default": "/usr/bin/ld"
	})json"_ojson;

	ret[kDefinitions]["toolchains-make"] = R"json({
		"type": "string",
		"description": "The executable path to GNU make utility.",
		"default": "/usr/bin/make"
	})json"_ojson;

	ret[kDefinitions]["toolchains-ninja"] = R"json({
		"type": "string",
		"description": "The executable path to ninja."
	})json"_ojson;

	ret[kDefinitions]["toolchains-objdump"] = R"json({
		"type": "string",
		"description": "The executable path to objdump."
	})json"_ojson;

	/*
	// These ancillaryTools don't get called directly (yet), but might be useful to look into
	ret[kDefinitions]["toolchains-ranlib"] = R"json({
		"type": "string",
		"description": "The executable path to ranlib.",
		"default": "/usr/bin/ranlib"
	})json"_ojson;

	ret[kDefinitions]["toolchains-strip"] = R"json({
		"type": "string",
		"description": "The executable path to strip.",
		"default": "/usr/bin/strip"
	})json"_ojson;
	*/

	ret[kDefinitions]["toolchains-windowsResource"] = R"json({
		"type": "string",
		"description": "The executable path to the resource compiler (Windows)"
	})json"_ojson;

	ret[kDefinitions]["toolchains-strategy"] = R"json({
		"type": "string",
		"description": "The build strategy to use.",
		"enum": [
			"makefile",
			"native-experimental",
			"ninja"
		],
		"default": "makefile"
	})json"_ojson;

	ret[kDefinitions]["toolchains"] = R"json({
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
		],
		"properties": {
			"strategy": {
				"$ref": "#/definitions/toolchains-strategy"
			},
			"archiver": {
				"$ref": "#/definitions/toolchains-archiver"
			},
			"C++": {
				"$ref": "#/definitions/toolchains-cpp"
			},
			"C": {
				"$ref": "#/definitions/toolchains-c"
			},
			"cmake": {
				"$ref": "#/definitions/toolchains-cmake"
			},
			"linker": {
				"$ref": "#/definitions/toolchains-linker"
			},
			"make": {
				"$ref": "#/definitions/toolchains-make"
			},
			"ninja": {
				"$ref": "#/definitions/toolchains-ninja"
			},
			"objdump": {
				"$ref": "#/definitions/toolchains-objdump"
			},
			"windowsResource": {
				"$ref": "#/definitions/toolchains-windowsResource"
			}
		}
	})json"_ojson;

	ret[kDefinitions]["settings-dumpAssembly"] = R"json({
		"type": "boolean",
		"description": "true to use include an asm dump of each file in the build, false otherwise.",
		"default": false
	})json"_ojson;

	ret[kDefinitions]["settings-maxJobs"] = R"json({
		"type": "integer",
		"description": "The number of threads to run during compilation. If this number exceeds the capabilities of the processor, the processor's max will be used.",
		"minimum": 1
	})json"_ojson;

	ret[kDefinitions]["settings-showCommands"] = R"json({
		"description": "true to show the commands run during the build, false to just show the source file.",
		"type": "boolean",
		"default": false
	})json"_ojson;

	ret[kDefinitions]["settings-toolchain"] = R"json({
		"description": "The toolchain id to use for building, if not the previous one.",
		"type": "string"
	})json"_ojson;

	ret[kDefinitions]["settings-macosSigningIdentity"] = R"json({
		"description": "The signing identity to use when bundling the macos application bundle.",
		"type": "string"
	})json"_ojson;

	//
	const auto kProperties = "properties";
	ret[kProperties] = Json::object();

	ret[kProperties]["ancillaryTools"] = R"json({
		"type": "object",
		"additionalProperties": false,
		"description": "The list of ancillaryTools for the platform",
		"properties": {
			"bash": {
				"$ref": "#/definitions/ancillaryTools-bash"
			},
			"brew": {
				"$ref": "#/definitions/ancillaryTools-brew"
			},
			"command_prompt": {
				"$ref": "#/definitions/ancillaryTools-command_prompt"
			},
			"codesign": {
				"$ref": "#/definitions/ancillaryTools-codesign"
			},
			"git": {
				"$ref": "#/definitions/ancillaryTools-git"
			},
			"gprof": {
				"$ref": "#/definitions/ancillaryTools-gprof"
			},
			"hdiutil": {
				"$ref": "#/definitions/ancillaryTools-hdiutil"
			},
			"install_name_tool": {
				"$ref": "#/definitions/ancillaryTools-install_name_tool"
			},
			"instruments": {
				"$ref": "#/definitions/ancillaryTools-instruments"
			},
			"ldd": {
				"$ref": "#/definitions/ancillaryTools-ldd"
			},
			"lipo": {
				"$ref": "#/definitions/ancillaryTools-lipo"
			},
			"lua": {
				"$ref": "#/definitions/ancillaryTools-lua"
			},
			"osascript": {
				"$ref": "#/definitions/ancillaryTools-osascript"
			},
			"otool": {
				"$ref": "#/definitions/ancillaryTools-otool"
			},
			"perl": {
				"$ref": "#/definitions/ancillaryTools-perl"
			},
			"plutil": {
				"$ref": "#/definitions/ancillaryTools-plutil"
			},
			"powershell": {
				"$ref": "#/definitions/ancillaryTools-powershell"
			},
			"python": {
				"$ref": "#/definitions/ancillaryTools-python"
			},
			"python3": {
				"$ref": "#/definitions/ancillaryTools-python3"
			},
			"ruby": {
				"$ref": "#/definitions/ancillaryTools-ruby"
			},
			"sample": {
				"$ref": "#/definitions/ancillaryTools-sample"
			},
			"sips": {
				"$ref": "#/definitions/ancillaryTools-sips"
			},
			"tiffutil": {
				"$ref": "#/definitions/ancillaryTools-tiffutil"
			},
			"xcodebuild": {
				"$ref": "#/definitions/ancillaryTools-xcodebuild"
			},
			"xcodegen": {
				"$ref": "#/definitions/ancillaryTools-xcodegen"
			},
			"xcrun": {
				"$ref": "#/definitions/ancillaryTools-xcrun"
			}
		}
	})json"_ojson;

	ret[kProperties]["applePlatformSdks"] = R"json({
		"type": "object",
		"description": "A list of Apple platform SDK paths (MacOS)"
	})json"_ojson;

	ret[kProperties]["workingDirectory"] = R"json({
		"type": "string",
		"description": "The working directory of the workspace"
	})json"_ojson;

	ret[kProperties]["externalDependencies"] = R"json({
		"type": "object",
		"description": "The external dependency cache"
	})json"_ojson;

	ret[kProperties]["settings"] = R"json({
		"type": "object",
		"additionalProperties": false,
		"description": "A list of settings central to the build",
		"required": [
			"dumpAssembly",
			"maxJobs",
			"showCommands",
			"toolchain",
			"macosSigningIdentity"
		],
		"properties": {
			"dumpAssembly": {
				"$ref": "#/definitions/settings-dumpAssembly"
			},
			"maxJobs": {
				"$ref": "#/definitions/settings-maxJobs"
			},
			"showCommands": {
				"$ref": "#/definitions/settings-showCommands"
			},
			"toolchain": {
				"$ref": "#/definitions/settings-toolchain"
			},
			"macosSigningIdentity": {
				"$ref": "#/definitions/settings-macosSigningIdentity"
			}
		}
	})json"_ojson;

	ret[kProperties]["toolchains"] = R"json({
		"type": "object",
		"additionalProperties": false,
		"description": "A list of compiler toolchains, mapped to the ids: msvc, llvm, or gcc",
		"patternProperties": {
			"^[\\w\\-\\+\\.]{3,}$": {
				"$ref": "#/definitions/toolchains"
			}
		}
	})json"_ojson;

	return ret;
}
}
