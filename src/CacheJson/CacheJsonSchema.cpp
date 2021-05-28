/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "CacheJson/CacheJsonSchema.hpp"

#include "Utility/String.hpp"
#include "Utility/SuppressIntellisense.hpp"
#include "Json/JsonComments.hpp"

namespace chalet
{
/*****************************************************************************/
Json Schema::getCacheJson()
{
	// Note: By parsing json from a string instead of _ojson literal, we can use ordered_json

	Json ret;
	ret["$schema"] = "http://json-schema.org/draft-07/schema";
	ret["type"] = "object";
	ret["additionalProperties"] = false;
	ret["required"] = {
		"settings",
		"tools",
		"compilerTools"
	};

	//
	const auto kDefinitions = "definitions";
	ret[kDefinitions] = Json::object();

	ret[kDefinitions]["tools-bash"] = R"json({
		"type": "string",
		"description": "The executable path to GNU bash",
		"default": "/usr/bin/bash"
	})json"_ojson;

	ret[kDefinitions]["tools-brew"] = R"json({
		"type": "string",
		"description": "The executable path to brew command-line utility (MacOS)",
		"default": "/usr/local/bin/brew"
	})json"_ojson;

	ret[kDefinitions]["tools-cmake"] = R"json({
		"type": "string",
		"description": "The executable path to CMake",
		"default": "/usr/local/bin/cmake"
	})json"_ojson;

	ret[kDefinitions]["tools-command_prompt"] = R"json({
		"type": "string",
		"description": "The executable path to Command Prompt (Windows)",
		"default": "C:/Windows/System32/cmd.exe"
	})json"_ojson;

	ret[kDefinitions]["tools-codesign"] = R"json({
		"type": "string",
		"description": "The executable path to codesign command-line utility (MacOS)",
		"default": "/usr/bin/codesign"
	})json"_ojson;

	ret[kDefinitions]["tools-git"] = R"json({
		"type": "string",
		"description": "The executable path to git.",
		"default": "/usr/bin/git"
	})json"_ojson;

	ret[kDefinitions]["tools-gprof"] = R"json({
		"type": "string",
		"description": "The executable path to gprof.",
		"default": "/usr/bin/gprof"
	})json"_ojson;

	ret[kDefinitions]["tools-hdiutil"] = R"json({
		"type": "string",
		"description": "The executable path to Apple's hdiutil command-line utility (MacOS)",
		"default": "/usr/bin/hdiutil"
	})json"_ojson;

	ret[kDefinitions]["tools-install_name_tool"] = R"json({
		"type": "string",
		"description": "The executable path to Apple's install_name_tool command-line utility (MacOS)",
		"default": "/usr/bin/install_name_tool"
	})json"_ojson;

	ret[kDefinitions]["tools-instruments"] = R"json({
		"type": "string",
		"description": "The executable path to Apple's instruments command-line utility (MacOS)",
		"default": "/usr/bin/instruments"
	})json"_ojson;

	ret[kDefinitions]["tools-ldd"] = R"json({
		"type": "string",
		"description": "The executable path to ldd.",
		"default": "/usr/bin/ldd"
	})json"_ojson;

	ret[kDefinitions]["tools-lua"] = R"json({
		"type": "string",
		"description": "The executable path to Lua",
		"default": "/usr/local/bin/lua"
	})json"_ojson;

	ret[kDefinitions]["tools-make"] = R"json({
		"type": "string",
		"description": "The executable path to GNU make utility.",
		"default": "/usr/bin/make"
	})json"_ojson;

	ret[kDefinitions]["tools-ninja"] = R"json({
		"type": "string",
		"description": "The executable path to ninja."
	})json"_ojson;

	ret[kDefinitions]["tools-objdump"] = R"json({
		"type": "string",
		"description": "The executable path to objdump."
	})json"_ojson;

	ret[kDefinitions]["tools-osascript"] = R"json({
		"type": "string",
		"description": "The executable path to Apple's osascript command-line utility (MacOS)",
		"default": "/usr/bin/osascript"
	})json"_ojson;

	ret[kDefinitions]["tools-otool"] = R"json({
		"type": "string",
		"description": "The executable path to Apple's otool command-line utility (MacOS)",
		"default": "/usr/bin/otool"
	})json"_ojson;

	ret[kDefinitions]["tools-perl"] = R"json({
		"type": "string",
		"description": "The executable path to Perl",
		"default": "/usr/local/bin/perl"
	})json"_ojson;

	ret[kDefinitions]["tools-plutil"] = R"json({
		"type": "string",
		"description": "The executable path to Apple's plutil command-line utility (MacOS)",
		"default": "/usr/bin/plutil"
	})json"_ojson;

	ret[kDefinitions]["tools-powershell"] = R"json({
		"type": "string",
		"description": "The executable path to Powershell (Windows)",
		"default": "C:/Windows/System32/WindowsPowerShell/v1.0/powershell.exe"
	})json"_ojson;

	ret[kDefinitions]["tools-python"] = R"json({
		"type": "string",
		"description": "The executable path to Python 2",
		"default": "/usr/bin/python"
	})json"_ojson;

	ret[kDefinitions]["tools-python3"] = R"json({
		"type": "string",
		"description": "The executable path to Python 3",
		"default": "/usr/local/bin/python3"
	})json"_ojson;

	ret[kDefinitions]["tools-ruby"] = R"json({
		"type": "string",
		"description": "The executable path to ruby.",
		"default": "/usr/bin/ruby"
	})json"_ojson;

	ret[kDefinitions]["tools-sample"] = R"json({
		"type": "string",
		"description": "The executable path to Apple's sample command-line utility (MacOS)",
		"default": "/usr/bin/sample"
	})json"_ojson;

	ret[kDefinitions]["tools-sips"] = R"json({
		"type": "string",
		"description": "The executable path to Apple's sips command-line utility (MacOS)",
		"default": "/usr/bin/sips"
	})json"_ojson;

	ret[kDefinitions]["tools-tiffutil"] = R"json({
		"type": "string",
		"description": "The executable path to tiffutil",
		"default": "/usr/bin/tiffutil"
	})json"_ojson;

	ret[kDefinitions]["tools-xcodebuild"] = R"json({
		"type": "string",
		"description": "The executable path to Apple's xcodebuild command-line utility (MacOS)",
		"default": "/usr/bin/xcodebuild"
	})json"_ojson;

	ret[kDefinitions]["tools-xcodegen"] = R"json({
		"type": "string",
		"description": "The executable path to the xcodegen Homebrew application (MacOS)",
		"default": "/usr/local/bin/xcodegen"
	})json"_ojson;

	ret[kDefinitions]["tools-xcrun"] = R"json({
		"type": "string",
		"description": "The executable path to Apple's xcrun command-line utility (MacOS)",
		"default": "/usr/bin/xcrun"
	})json"_ojson;

	// libtool (macOS), ar (Linux / macOS / MinGW), lib.exe (Win)
	ret[kDefinitions]["compilerTools-archiver"] = R"json({
		"type": "string",
		"description": "The executable path to the toolchain's static library archive utility",
		"default": "/usr/bin/ar"
	})json"_ojson;

	ret[kDefinitions]["compilerTools-cpp"] = R"json({
		"type": "string",
		"description": "The executable path to the toolchain's C++ compiler",
		"default": "/usr/bin/c++"
	})json"_ojson;

	ret[kDefinitions]["compilerTools-c"] = R"json({
		"type": "string",
		"description": "The executable path to the toolchain's C compiler",
		"default": "/usr/bin/cc"
	})json"_ojson;

	ret[kDefinitions]["compilerTools-linker"] = R"json({
		"type": "string",
		"description": "The executable path to the toolchain's linker",
		"default": "/usr/bin/ld"
	})json"_ojson;

	/*
	// These tools don't get called directly (yet), but might be useful to look into
	ret[kDefinitions]["compilerTools-ranlib"] = R"json({
		"type": "string",
		"description": "The executable path to ranlib.",
		"default": "/usr/bin/ranlib"
	})json"_ojson;

	ret[kDefinitions]["compilerTools-strip"] = R"json({
		"type": "string",
		"description": "The executable path to strip.",
		"default": "/usr/bin/strip"
	})json"_ojson;
	*/

	ret[kDefinitions]["compilerTools-windowsResource"] = R"json({
		"type": "string",
		"description": "The executable path to the resource compiler (Windows)"
	})json"_ojson;

	ret[kDefinitions]["environment-path"] = R"json({
		"type": "array",
		"description": "Any additional paths to include not otherwise in the PATH environment variable.",
		"uniqueItems": true,
		"items": {
			"type": "string"
		}
	})json"_ojson;

	/*ret[kDefinitions]["settings-targetArchitecture"] = R"json({
		"type": "string",
		"description": "The target platform's architecture",
		"enum": [
			"x86",
			"x64",
			"arm",
			"arm64"
		],
		"default": "x64"
	})json"_ojson;*/

	ret[kDefinitions]["settings-strategy"] = R"json({
		"type": "string",
		"description": "The build strategy to use.",
		"enum": [
			"makefile",
			"native-experimental",
			"ninja"
		],
		"default": "makefile"
	})json"_ojson;

	//
	const auto kProperties = "properties";
	ret[kProperties] = Json::object();

	ret[kProperties]["tools"] = R"json({
		"type": "object",
		"additionalProperties": false,
		"description": "The list of tools for the platform",
		"required": [
			"bash",
			"brew",
			"command_prompt",
			"cmake",
			"codesign",
			"git",
			"gprof",
			"hdiutil",
			"install_name_tool",
			"instruments",
			"ldd",
			"lua",
			"make",
			"ninja",
			"objdump",
			"osascript",
			"otool",
			"perl",
			"plutil",
			"powershell",
			"python",
			"python3",
			"ruby",
			"sample",
			"sips",
			"tiffutil",
			"xcodebuild",
			"xcodegen",
			"xcrun"
		],
		"properties": {
			"bash": {
				"$ref": "#/definitions/tools-bash"
			},
			"brew": {
				"$ref": "#/definitions/tools-brew"
			},
			"cmake": {
				"$ref": "#/definitions/tools-cmake"
			},
			"command_prompt": {
				"$ref": "#/definitions/tools-command_prompt"
			},
			"codesign": {
				"$ref": "#/definitions/tools-codesign"
			},
			"git": {
				"$ref": "#/definitions/tools-git"
			},
			"gprof": {
				"$ref": "#/definitions/tools-gprof"
			},
			"hdiutil": {
				"$ref": "#/definitions/tools-hdiutil"
			},
			"install_name_tool": {
				"$ref": "#/definitions/tools-install_name_tool"
			},
			"instruments": {
				"$ref": "#/definitions/tools-instruments"
			},
			"ldd": {
				"$ref": "#/definitions/tools-ldd"
			},
			"lua": {
				"$ref": "#/definitions/tools-lua"
			},
			"make": {
				"$ref": "#/definitions/tools-make"
			},
			"ninja": {
				"$ref": "#/definitions/tools-ninja"
			},
			"objdump": {
				"$ref": "#/definitions/tools-objdump"
			},
			"osascript": {
				"$ref": "#/definitions/tools-osascript"
			},
			"otool": {
				"$ref": "#/definitions/tools-otool"
			},
			"perl": {
				"$ref": "#/definitions/tools-perl"
			},
			"plutil": {
				"$ref": "#/definitions/tools-plutil"
			},
			"powershell": {
				"$ref": "#/definitions/tools-powershell"
			},
			"python": {
				"$ref": "#/definitions/tools-python"
			},
			"python3": {
				"$ref": "#/definitions/tools-python3"
			},
			"ruby": {
				"$ref": "#/definitions/tools-ruby"
			},
			"sample": {
				"$ref": "#/definitions/tools-sample"
			},
			"sips": {
				"$ref": "#/definitions/tools-sips"
			},
			"tiffutil": {
				"$ref": "#/definitions/tools-tiffutil"
			},
			"xcodebuild": {
				"$ref": "#/definitions/tools-xcodebuild"
			},
			"xcodegen": {
				"$ref": "#/definitions/tools-xcodegen"
			},
			"xcrun": {
				"$ref": "#/definitions/tools-xcrun"
			}
		}
	})json"_ojson;

	ret[kProperties]["applePlatformSdks"] = R"json({
		"type": "object",
		"description": "A list of Apple platform SDK paths (MacOS)"
	})json"_ojson;

	ret[kProperties]["compilerTools"] = R"json({
		"type": "object",
		"additionalProperties": false,
		"description": "The list of compilers for the platform",
		"required": [
			"archiver",
			"C++",
			"C",
			"linker",
			"windowsResource"
		],
		"properties": {
			"archiver": {
				"$ref": "#/definitions/compilerTools-archiver"
			},
			"C++": {
				"$ref": "#/definitions/compilerTools-cpp"
			},
			"C": {
				"$ref": "#/definitions/compilerTools-c"
			},
			"linker": {
				"$ref": "#/definitions/compilerTools-linker"
			},
			"windowsResource": {
				"$ref": "#/definitions/compilerTools-windowsResource"
			}
		}
	})json"_ojson;

	ret[kProperties]["workingDirectory"] = R"json({
		"type": "string",
		"description": "The working directory of the workspace"
	})json"_ojson;

	ret[kProperties]["externalDependencies"] = R"json({
		"type": "object",
		"description": "The external dependency cache"
	})json"_ojson;

	ret[kProperties]["data"] = R"json({
			"type": "object"
	})json"_ojson;

	ret[kProperties]["settings"] = R"json({
		"type": "object",
		"additionalProperties": false,
		"description": "A list of settings central to the build",
		"required": [
			"strategy"
		],
		"properties": {
			"strategy": {
				"$ref": "#/definitions/settings-strategy"
			}
		}
	})json"_ojson;

	return ret;
}
}
