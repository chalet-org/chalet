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
		"tools",
		"compilers"
	};

	//
	const auto kDefinitions = "definitions";
	ret[kDefinitions] = Json::object();

	ret[kDefinitions]["tools-ar"] = R"json({
		"type": "string",
		"description": "The executable path to GNU ar",
		"default": "/usr/bin/ar"
	})json"_ojson;

	ret[kDefinitions]["tools-brew"] = R"json({
		"type": "string",
		"description": "The executable path to brew (MacOS)",
		"default": "/usr/local/bin/brew"
	})json"_ojson;

	ret[kDefinitions]["tools-cmake"] = R"json({
		"type": "string",
		"description": "The executable path to CMake",
		"default": "/usr/local/bin/cmake"
	})json"_ojson;

	ret[kDefinitions]["tools-codesign"] = R"json({
		"type": "string",
		"description": "The executable path to codesign (MacOS",
		"default": "/usr/bin/codesign"
	})json"_ojson;

	ret[kDefinitions]["tools-git"] = R"json({
		"type": "string",
		"description": "The executable path to git.",
		"default": "/usr/bin/git"
	})json"_ojson;

	ret[kDefinitions]["tools-hdiutil"] = R"json({
		"type": "string",
		"description": "The executable path to Apple's hdiutil (MacOS)",
		"default": "/usr/bin/hdiutil"
	})json"_ojson;

	ret[kDefinitions]["tools-install_name_tool"] = R"json({
		"type": "string",
		"description": "The executable path to Apple's install_name_tool (MacOS)",
		"default": "/usr/bin/install_name_tool"
	})json"_ojson;

	ret[kDefinitions]["tools-ldd"] = R"json({
		"type": "string",
		"description": "The executable path to ldd.",
		"default": "/usr/bin/ldd"
	})json"_ojson;

	ret[kDefinitions]["tools-macosSdk"] = R"json({
		"type": "string",
		"description": "The path to the current MacOS SDK (MacOS)"
	})json"_ojson;

	ret[kDefinitions]["tools-make"] = R"json({
		"type": "string",
		"description": "The executable path to GNU make utility.",
		"default": "/usr/bin/make"
	})json"_ojson;

	ret[kDefinitions]["tools-ninja"] = R"json({
		"type": "string",
		"description": "The path to ninja's executable."
	})json"_ojson;

	ret[kDefinitions]["tools-otool"] = R"json({
		"type": "string",
		"description": "The executable path to Apple's otool (MacOS)",
		"default": "/usr/bin/otool"
	})json"_ojson;

	ret[kDefinitions]["tools-plutil"] = R"json({
		"type": "string",
		"description": "The executable path to Apple's plutil (MacOS)",
		"default": "/usr/bin/plutil"
	})json"_ojson;

	ret[kDefinitions]["tools-ranlib"] = R"json({
		"type": "string",
		"description": "The executable path to ranlib.",
		"default": "/usr/bin/ranlib"
	})json"_ojson;

	ret[kDefinitions]["tools-sips"] = R"json({
		"type": "string",
		"description": "The executable path to Apple's sips utility (MacOS)",
		"default": "/usr/bin/sips"
	})json"_ojson;

	ret[kDefinitions]["tools-strip"] = R"json({
		"type": "string",
		"description": "The executable path to strip.",
		"default": "/usr/bin/strip"
	})json"_ojson;

	ret[kDefinitions]["tools-tiffutil"] = R"json({
		"type": "string",
		"description": "The executable path to tiffutil",
		"default": "/usr/bin/tiffutil"
	})json"_ojson;

	ret[kDefinitions]["compilers-cpp"] = R"json({
		"type": "string",
		"description": "The executable path to the C++ compiler",
		"default": "/usr/bin/c++"
	})json"_ojson;

	ret[kDefinitions]["compilers-c"] = R"json({
		"type": "string",
		"description": "The executable path to the C compiler",
		"default": "/usr/bin/cc"
	})json"_ojson;

	ret[kDefinitions]["compilers-windowsResource"] = R"json({
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

	ret[kDefinitions]["environment-architecture"] = R"json({
		"type": "string",
		"description": "The target platform architecture",
		"enum": [
			"auto",
			"x86",
			"x64",
			"ARM",
			"ARM64"
		],
		"default": "auto"
	})json"_ojson;

	ret[kDefinitions]["environment-strategy"] = R"json({
		"type": "string",
		"description": "The build strategy to use.",
		"enum": [
			"makefile",
			"native-experimental",
			"ninja-experimental"
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
			"ar",
			"brew",
			"cmake",
			"codesign",
			"git",
			"hdiutil",
			"install_name_tool",
			"ldd",
			"macosSdk",
			"make",
			"ninja",
			"otool",
			"plutil",
			"ranlib",
			"sips",
			"strip",
			"tiffutil"
		],
		"properties": {
			"ar": {
				"$ref": "#/definitions/tools-ar"
			},
			"brew": {
				"$ref": "#/definitions/tools-brew"
			},
			"cmake": {
				"$ref": "#/definitions/tools-cmake"
			},
			"codesign": {
				"$ref": "#/definitions/tools-codesign"
			},
			"git": {
				"$ref": "#/definitions/tools-git"
			},
			"hdiutil": {
				"$ref": "#/definitions/tools-hdiutil"
			},
			"install_name_tool": {
				"$ref": "#/definitions/tools-install_name_tool"
			},
			"ldd": {
				"$ref": "#/definitions/tools-ldd"
			},
			"macosSdk": {
				"$ref": "#/definitions/tools-macosSdk"
			},
			"make": {
				"$ref": "#/definitions/tools-make"
			},
			"ninja": {
				"$ref": "#/definitions/tools-ninja"
			},
			"otool": {
				"$ref": "#/definitions/tools-otool"
			},
			"plutil": {
				"$ref": "#/definitions/tools-plutil"
			},
			"ranlib": {
				"$ref": "#/definitions/tools-ranlib"
			},
			"sips": {
				"$ref": "#/definitions/tools-sips"
			},
			"strip": {
				"$ref": "#/definitions/tools-strip"
			},
			"tiffutil": {
				"$ref": "#/definitions/tools-tiffutil"
			}
		}
	})json"_ojson;

	ret[kProperties]["compilers"] = R"json({
		"type": "object",
		"additionalProperties": false,
		"description": "The list of compilers for the platform",
		"required": [
			"C++",
			"C",
			"windowsResource"
		],
		"properties": {
			"C++": {
				"$ref": "#/definitions/compilers-cpp"
			},
			"C": {
				"$ref": "#/definitions/compilers-c"
			},
			"windowsResource": {
				"$ref": "#/definitions/compilers-windowsResource"
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

	ret[kProperties]["strategy"] = R"json({
			"$ref": "#/definitions/environment-strategy"
	})json"_ojson;

	return ret;
}
}
