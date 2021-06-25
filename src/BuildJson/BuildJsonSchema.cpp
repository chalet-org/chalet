/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "BuildJson/BuildJsonSchema.hpp"

#include "FileTemplates/PlatformFileTemplates.hpp"

#include "Utility/String.hpp"
#include "Utility/SuppressIntellisense.hpp"
#include "Json/JsonComments.hpp"

namespace chalet
{
/*****************************************************************************/
Json Schema::getBuildJson()
{
	const std::string patternProjectName = R"(^[\w\-\+\.]{3,}$)";
	const std::string patternProjectLinks = R"(^[\w\-\+\.]+$)";
	const std::string patternDistributionName = R"(^[\w\-\+\.\ \(\)]{3,}$)";

	const std::string patternConfigurations = R"((:debug|:!debug|))";
	const std::string patternPlatforms = R"((\.windows|\.macos|\.linux|\.\!windows|\.\!macos|\.\!linux|))";

	Json ret;
	ret["$schema"] = "http://json-schema.org/draft-07/schema";
	ret["type"] = "object";
	ret["additionalProperties"] = false;
	ret["required"] = {
		"version",
		"workspace",
		"targets"
	};

	//
	const auto kItems = "items";
	const auto kPattern = "pattern";
	const auto kPatternProperties = "patternProperties";
	const auto kEnum = "enum";
	const auto kAnyOf = "anyOf";

	//
	const auto kDefinitions = "definitions";
	ret[kDefinitions] = Json::object();

	// configurations
	ret[kDefinitions]["configurations-debugSymbols"] = R"json({
		"type": "boolean",
		"description": "true to include debug symbols, false otherwise.",
		"default": false
	})json"_ojson;

	ret[kDefinitions]["configurations-enableProfiling"] = R"json({
		"type": "boolean",
		"description": "true to enable profiling for this configuration, false otherwise.",
		"default": false
	})json"_ojson;

	ret[kDefinitions]["configurations-linkTimeOptimization"] = R"json({
		"type": "boolean",
		"description": "true to use link-time optimization, false otherwise.",
		"default": false
	})json"_ojson;

	ret[kDefinitions]["configurations-optimizations"] = R"json({
		"type": "string",
		"description": "The optimization level of the build.",
		"enum": [
			"0",
			"1",
			"2",
			"3",
			"debug",
			"size",
			"fast"
		]
	})json"_ojson;

	ret[kDefinitions]["configurations-stripSymbols"] = R"json({
		"type": "boolean",
		"description": "true to strip symbols from the build, false otherwise.",
		"default": false
	})json"_ojson;

	// distribution
	ret[kDefinitions]["distribution-configuration"] = R"json({
		"type": "string",
		"description": "The name of the build configuration to use for the distribution.",
		"default": "Release"
	})json"_ojson;

	ret[kDefinitions]["distribution-dependencies"] = R"json({
		"type": "array",
		"uniqueItems": true,
		"minItems": 1,
		"items": {
			"type": "string"
		}
	})json"_ojson;

	ret[kDefinitions]["distribution-description"] = R"json({
		"type": "string"
	})json"_ojson;

	ret[kDefinitions]["distribution-exclude"] = R"json({
		"type": "array",
		"uniqueItems": true,
		"minItems": 1,
		"items": {
			"type": "string"
		}
	})json"_ojson;

	ret[kDefinitions]["distribution-includeDependentSharedLibraries"] = R"json({
		"type": "boolean",
		"default": true
	})json"_ojson;

	ret[kDefinitions]["distribution-linux"] = R"json({
		"type": "object",
		"description": "Variables to describe the linux application.",
		"additionalProperties": false,
		"required": [
			"icon",
			"desktopEntry"
		],
		"properties": {
			"desktopEntry": {
				"type": "string",
				"description": "The location to an XDG Desktop Entry template. If the file does not exist, it will be generated."
			},
			"icon": {
				"type": "string",
				"description": "The location to an icon to use for the application (PNG 256x256 is recommended)"
			}
		}
	})json"_ojson;

	ret[kDefinitions]["distribution-macos"] = R"json({
		"type": "object",
		"description": "Variables to describe the macos application bundle.",
		"additionalProperties": false,
		"properties": {
			"bundleName": {
				"type": "string"
			},
			"dmgBackground": {
				"anyOf": [
					{
						"type": "string"
					},
					{
						"type": "object",
						"required": [
							"1x"
						],
						"properties": {
							"1x": {
								"type": "string"
							},
							"2x": {
								"type": "string"
							}
						}
					}
				]
			},
			"icon": {
				"type": "string"
			},
			"infoPropertyList": {
				"anyOf": [
					{
						"type": "string"
					},
					{
						"type": "object"
					}
				]
			},
			"universalBinary": {
				"type": "boolean",
				"description": "If true, the project will be built in both x64 and arm64, and combined into universal binaries before being bundled.",
				"default": false
			},
			"makeDmg": {
				"type": "boolean",
				"description": "If true, a .dmg image will be built",
				"default": false
			}
		}
	})json"_ojson;
	ret[kDefinitions]["distribution-macos"]["properties"]["infoPropertyList"]["anyOf"][1]["default"] = JsonComments::parseLiteral(PlatformFileTemplates::macosInfoPlist());

	ret[kDefinitions]["distribution-mainProject"] = R"json({
		"type": "string",
		"description": "The main executable project."
	})json"_ojson;

	ret[kDefinitions]["distribution-outDir"] = R"json({
		"type": "string",
		"description": "The output folder to place the final build along with all of its dependencies.",
		"default": "dist"
	})json"_ojson;

	ret[kDefinitions]["distribution-projects"] = R"json({
		"type": "array",
		"uniqueItems": true,
		"description": "An array of projects to include",
		"minItems": 1,
		"items": {
			"type": "string",
			"description": "The name of the project"
		}
	})json"_ojson;
	ret[kDefinitions]["distribution-projects"][kItems][kPattern] = patternProjectName;

	ret[kDefinitions]["distribution-windows"] = R"json({
		"type": "object",
		"description": "Variables to describe the windows application.",
		"additionalProperties": false,
		"required": [],
		"properties": {}
	})json"_ojson;

	ret[kDefinitions]["distribution-bundle"] = R"json({
		"type": "object",
		"additionalProperties": false,
		"description": "Variables to describe the final output build.",
		"properties": {
			"configuration": {
				"$ref": "#/definitions/distribution-configuration"
			},
			"dependencies": {
				"$ref": "#/definitions/distribution-dependencies"
			},
			"description": {
				"$ref": "#/definitions/distribution-description"
			},
			"exclude": {
				"$ref": "#/definitions/distribution-exclude"
			},
			"includeDependentSharedLibraries": {
				"$ref": "#/definitions/distribution-includeDependentSharedLibraries"
			},
			"linux": {
				"$ref": "#/definitions/distribution-linux"
			},
			"macos": {
				"$ref": "#/definitions/distribution-macos"
			},
			"mainProject": {
				"$ref": "#/definitions/distribution-mainProject"
			},
			"outDir": {
				"$ref": "#/definitions/distribution-outDir"
			},
			"projects": {
				"$ref": "#/definitions/distribution-projects"
			}
		}
	})json"_ojson;
	ret[kDefinitions]["distribution-bundle"][kPatternProperties][fmt::format("^dependencies{}{}$", patternConfigurations, patternPlatforms)] = R"json({
		"$ref": "#/definitions/distribution-dependencies"
	})json"_ojson;
	ret[kDefinitions]["distribution-bundle"][kPatternProperties][fmt::format("^exclude{}{}$", patternConfigurations, patternPlatforms)] = R"json({
		"$ref": "#/definitions/distribution-exclude"
	})json"_ojson;

	// externalDependency
	ret[kDefinitions]["externalDependency-repository"] = R"json({
		"type": "string",
		"description": "The url of the git repository.",
		"pattern": "^(?:git|ssh|https?|git@[-\\w.]+):(\\/\\/)?(.*?)(\\.git)(\\/?|\\#[-\\d\\w._]+?)$"
	})json"_ojson;

	ret[kDefinitions]["externalDependency-branch"] = R"json({
		"type": "string",
		"description": "The branch to checkout. Uses the repository's default if not set."
	})json"_ojson;

	ret[kDefinitions]["externalDependency-commit"] = R"json({
		"type": "string",
		"description": "The SHA1 hash of the commit to checkout.",
		"pattern": "^[0-9a-f]{7,40}$"
	})json"_ojson;

	ret[kDefinitions]["externalDependency-tag"] = R"json({
		"type": "string",
		"description": "The tag to checkout on the selected branch. If it's blank or not found, the head of the branch will be checked out."
	})json"_ojson;

	ret[kDefinitions]["externalDependency-submodules"] = R"json({
		"type": "boolean",
		"description": "Do submodules need to be cloned?",
		"default": false
	})json"_ojson;

	ret[kDefinitions]["externalDependency"] = R"json({
		"type": "object",
		"oneOf": [
			{
				"additionalProperties": false,
				"required": [
					"repository",
					"tag"
				],
				"properties": {
					"repository": {
						"$ref": "#/definitions/externalDependency-repository"
					},
					"submodules": {
						"$ref": "#/definitions/externalDependency-submodules"
					},
					"tag": {
						"$ref": "#/definitions/externalDependency-tag"
					}
				}
			},
			{
				"additionalProperties": false,
				"required": [
					"repository"
				],
				"properties": {
					"repository": {
						"$ref": "#/definitions/externalDependency-repository"
					},
					"submodules": {
						"$ref": "#/definitions/externalDependency-submodules"
					},
					"branch": {
						"$ref": "#/definitions/externalDependency-branch"
					},
					"commit": {
						"$ref": "#/definitions/externalDependency-commit"
					}
				}
			}
		]
	})json"_ojson;

	ret[kDefinitions]["enum-platform"] = R"json({
		"type": "string",
		"enum": [
			"windows",
			"macos",
			"linux"
		]
	})json"_ojson;

	ret[kDefinitions]["environment-path"] = R"json({
		"type": "array",
		"description": "Any additional paths to include.",
		"uniqueItems": true,
		"minItems": 1,
		"items": {
			"type": "string"
		}
	})json"_ojson;

	ret[kDefinitions]["target-description"] = R"json({
		"type": "string",
		"description": "A description of the target to display during the build."
	})json"_ojson;

	ret[kDefinitions]["target-notInConfiguration"] = R"json({
		"description": "Don't compile this project in specific build configuration(s)",
		"oneOf": [
			{
				"type": "string"
			},
			{
				"type": "array",
				"uniqueItems": true,
				"minItems": 1,
				"items": {
					"type": "string"
				}
			}
		]
	})json"_ojson;

	ret[kDefinitions]["target-notInPlatform"] = R"json({
		"description": "Don't compile this project on specific platform(s)",
		"oneOf": [
			{
				"$ref": "#/definitions/enum-platform"
			},
			{
				"type": "array",
				"uniqueItems": true,
				"minItems": 1,
				"items": {
					"$ref": "#/definitions/enum-platform"
				}
			}
		]
	})json"_ojson;

	ret[kDefinitions]["target-onlyInConfiguration"] = R"json({
		"description": "Only compile this project in specific build configuration(s)",
		"oneOf": [
			{
				"type": "string"
			},
			{
				"type": "array",
				"uniqueItems": true,
				"minItems": 1,
				"items": {
					"type": "string"
				}
			}
		]
	})json"_ojson;

	ret[kDefinitions]["target-onlyInPlatform"] = R"json({
		"description": "Only compile this project on specific platform(s)",
		"oneOf": [
			{
				"$ref": "#/definitions/enum-platform"
			},
			{
				"type": "array",
				"uniqueItems": true,
				"minItems": 1,
				"items": {
					"$ref": "#/definitions/enum-platform"
				}
			}
		]
	})json"_ojson;

	ret[kDefinitions]["target-project-settings-cxx"] = R"json({
		"type": "object",
		"additionalProperties": false,
		"properties": {
			"cStandard": {
				"$ref": "#/definitions/target-project-cxx-cStandard"
			},
			"compileOptions": {
				"$ref": "#/definitions/target-project-cxx-compileOptions"
			},
			"cppStandard": {
				"$ref": "#/definitions/target-project-cxx-cppStandard"
			},
			"defines": {
				"$ref": "#/definitions/target-project-cxx-defines"
			},
			"includeDirs": {
				"$ref": "#/definitions/target-project-cxx-includeDirs"
			},
			"libDirs": {
				"$ref": "#/definitions/target-project-cxx-libDirs"
			},
			"linkerScript": {
				"$ref": "#/definitions/target-project-cxx-linkerScript"
			},
			"linkerOptions": {
				"$ref": "#/definitions/target-project-cxx-linkerOptions"
			},
			"links": {
				"$ref": "#/definitions/target-project-cxx-links"
			},
			"macosFrameworkPaths": {
				"$ref": "#/definitions/target-project-cxx-macosFrameworkPaths"
			},
			"macosFrameworks": {
				"$ref": "#/definitions/target-project-cxx-macosFrameworks"
			},
			"objectiveCxx": {
				"$ref": "#/definitions/target-project-cxx-objectiveCxx"
			},
			"pch": {
				"$ref": "#/definitions/target-project-cxx-pch"
			},
			"threads": {
				"$ref": "#/definitions/target-project-cxx-threads"
			},
			"rtti": {
				"$ref": "#/definitions/target-project-cxx-rtti"
			},
			"staticLinking": {
				"$ref": "#/definitions/target-project-cxx-staticLinking"
			},
			"staticLinks": {
				"$ref": "#/definitions/target-project-cxx-staticLinks"
			},
			"warnings": {
				"$ref": "#/definitions/target-project-cxx-warnings"
			},
			"windowsPrefixOutputFilename": {
				"$ref": "#/definitions/target-project-cxx-windowsPrefixOutputFilename"
			},
			"windowsOutputDef": {
				"$ref": "#/definitions/target-project-cxx-windowsOutputDef"
			},
			"windowsApplicationIcon": {
				"$ref": "#/definitions/target-project-cxx-windowsApplicationIcon"
			},
			"windowsApplicationManifest": {
				"$ref": "#/definitions/target-project-cxx-windowsApplicationManifest"
			}
		}
	})json"_ojson;

	ret[kDefinitions]["target-project-settings-cxx"][kPatternProperties][fmt::format("^cStandard{}$", patternPlatforms)] = R"json({
		"$ref": "#/definitions/target-project-cxx-cStandard"
	})json"_ojson;
	ret[kDefinitions]["target-project-settings-cxx"][kPatternProperties][fmt::format("^cppStandard{}$", patternPlatforms)] = R"json({
		"$ref": "#/definitions/target-project-cxx-cppStandard"
	})json"_ojson;
	ret[kDefinitions]["target-project-settings-cxx"][kPatternProperties][fmt::format("^compileOptions{}$", patternPlatforms)] = R"json({
		"$ref": "#/definitions/target-project-cxx-compileOptions"
	})json"_ojson;
	ret[kDefinitions]["target-project-settings-cxx"][kPatternProperties][fmt::format("^defines{}{}$", patternConfigurations, patternPlatforms)] = R"json({
		"$ref": "#/definitions/target-project-cxx-defines"
	})json"_ojson;
	ret[kDefinitions]["target-project-settings-cxx"][kPatternProperties][fmt::format("^includeDirs{}$", patternPlatforms)] = R"json({
		"$ref": "#/definitions/target-project-cxx-includeDirs"
	})json"_ojson;
	ret[kDefinitions]["target-project-settings-cxx"][kPatternProperties][fmt::format("^libDirs{}$", patternPlatforms)] = R"json({
		"$ref": "#/definitions/target-project-cxx-libDirs"
	})json"_ojson;
	ret[kDefinitions]["target-project-settings-cxx"][kPatternProperties][fmt::format("^linkerScript{}$", patternPlatforms)] = R"json({
		"$ref": "#/definitions/target-project-cxx-linkerScript"
	})json"_ojson;
	ret[kDefinitions]["target-project-settings-cxx"][kPatternProperties][fmt::format("^linkerOptions{}$", patternPlatforms)] = R"json({
		"$ref": "#/definitions/target-project-cxx-linkerOptions"
	})json"_ojson;
	ret[kDefinitions]["target-project-settings-cxx"][kPatternProperties][fmt::format("^links{}{}$", patternConfigurations, patternPlatforms)] = R"json({
		"$ref": "#/definitions/target-project-cxx-links"
	})json"_ojson;
	ret[kDefinitions]["target-project-settings-cxx"][kPatternProperties][fmt::format("^objectiveCxx{}$", patternPlatforms)] = R"json({
		"$ref": "#/definitions/target-project-cxx-objectiveCxx"
	})json"_ojson;
	ret[kDefinitions]["target-project-settings-cxx"][kPatternProperties][fmt::format("^staticLinks{}{}$", patternConfigurations, patternPlatforms)] = R"json({
		"$ref": "#/definitions/target-project-cxx-staticLinks"
	})json"_ojson;
	ret[kDefinitions]["target-project-settings-cxx"][kPatternProperties][fmt::format("^threads{}{}$", patternConfigurations, patternPlatforms)] = R"json({
		"$ref": "#/definitions/target-project-cxx-threads"
	})json"_ojson;
	ret[kDefinitions]["target-project-settings-cxx"][kPatternProperties][fmt::format("^rtti{}{}$", patternConfigurations, patternPlatforms)] = R"json({
		"$ref": "#/definitions/target-project-cxx-rtti"
	})json"_ojson;
	ret[kDefinitions]["target-project-settings-cxx"][kPatternProperties][fmt::format("^staticLinking{}{}$", patternConfigurations, patternPlatforms)] = R"json({
		"$ref": "#/definitions/target-project-cxx-staticLinking"
	})json"_ojson;

	ret[kDefinitions]["target-project"] = R"json({
		"type": "object",
		"additionalProperties": false,
		"properties": {
			"settings:Cxx": {
				"$ref": "#/definitions/target-project-settings-cxx"
			},
			"extends": {
				"$ref": "#/definitions/target-project-extends"
			},
			"files": {
				"$ref": "#/definitions/target-project-files"
			},
			"kind": {
				"$ref": "#/definitions/target-project-kind"
			},
			"language": {
				"$ref": "#/definitions/target-project-language"
			},
			"location": {
				"$ref": "#/definitions/target-project-location"
			},
			"onlyInConfiguration": {
				"$ref": "#/definitions/target-onlyInConfiguration"
			},
			"notInConfiguration": {
				"$ref": "#/definitions/target-notInConfiguration"
			},
			"onlyInPlatform": {
				"$ref": "#/definitions/target-onlyInPlatform"
			},
			"notInPlatform": {
				"$ref": "#/definitions/target-notInPlatform"
			},
			"runProject": {
				"$ref": "#/definitions/target-project-runProject"
			},
			"runArguments": {
				"$ref": "#/definitions/target-project-runArguments"
			},
			"runDependencies": {
				"$ref": "#/definitions/target-project-runDependencies"
			}
		}
	})json"_ojson;
	ret[kDefinitions]["target-project"][kPatternProperties][fmt::format("^runProject{}{}$", patternConfigurations, patternPlatforms)] = R"json({
		"$ref": "#/definitions/target-project-runProject"
	})json"_ojson;
	ret[kDefinitions]["target-project"][kPatternProperties][fmt::format("^runDependencies{}{}$", patternConfigurations, patternPlatforms)] = R"json({
		"$ref": "#/definitions/target-project-runDependencies"
	})json"_ojson;

	ret[kDefinitions]["target-project-cxx-cStandard"] = R"json({
		"type": "string",
		"description": "The C standard to use in the compilation",
		"pattern": "^((c|gnu)\\d[\\dx]|(iso9899:(1990|199409|1999|199x|20\\d{2})))$",
		"default": "c11"
	})json"_ojson;

	ret[kDefinitions]["target-project-cxx-compileOptions"] = R"json({
		"type": "array",
		"uniqueItems": true,
		"minItems": 1,
		"description": "Options to add during the compilation step.",
		"items": {
			"type": "string"
		}
	})json"_ojson;

	ret[kDefinitions]["target-project-cxx-cppStandard"] = R"json({
		"type": "string",
		"description": "The C++ standard to use in the compilation",
		"pattern": "^(c|gnu)\\+\\+\\d[\\dxyzab]$",
		"default": "c++17"
	})json"_ojson;

	ret[kDefinitions]["target-project-cxx-defines"] = R"json({
		"type": "array",
		"uniqueItems": true,
		"minItems": 1,
		"description": "Macro definitions to be used by the preprocessor",
		"items": {
			"type": "string"
		}
	})json"_ojson;

	ret[kDefinitions]["target-project-cxx-includeDirs"] = R"json({
		"type": "array",
		"uniqueItems": true,
		"minItems": 1,
		"description": "A list of directories to include with the project.",
		"items": {
			"type": "string"
		}
	})json"_ojson;

	ret[kDefinitions]["target-project-extends"] = R"json({
		"type": "string",
		"description": "A project template to extend. Defaults to 'all' implicitly.",
		"pattern": "^[A-Za-z_-]+$",
		"default": "all"
	})json"_ojson;

	ret[kDefinitions]["target-project-files"] = R"json({
		"type": "array",
		"uniqueItems": true,
		"minItems": 1,
		"description": "Explicitly define the source files, relative to the working directory.",
		"items": {
			"type": "string"
		}
	})json"_ojson;

	ret[kDefinitions]["target-project-kind"] = R"json({
		"type": "string",
		"description": "The type of the project's compiled binary.",
		"enum": [
			"staticLibrary",
			"sharedLibrary",
			"consoleApplication",
			"desktopApplication"
		]
	})json"_ojson;

	ret[kDefinitions]["target-project-language"] = R"json({
		"type": "string",
		"description": "The target language of the project.",
		"enum": [
			"C",
			"C++"
		],
		"default": "C++"
	})json"_ojson;

	ret[kDefinitions]["target-project-cxx-libDirs"] = R"json({
		"type": "array",
		"uniqueItems": true,
		"minItems": 1,
		"description": "Fallback search paths to look for static or dynamic libraries (/usr/lib is included by default)",
		"items": {
			"type": "string"
		}
	})json"_ojson;

	ret[kDefinitions]["target-project-cxx-linkerScript"] = R"json({
		"type": "string",
		"description": "An LD linker script path (.ld file) to pass to the linker command"
	})json"_ojson;

	ret[kDefinitions]["target-project-cxx-linkerOptions"] = R"json({
		"type": "array",
		"uniqueItems": true,
		"minItems": 1,
		"description": "Options to add during the linking step.",
		"items": {
			"type": "string"
		}
	})json"_ojson;

	ret[kDefinitions]["target-project-cxx-links"] = R"json({
		"type": "array",
		"uniqueItems": true,
		"minItems": 1,
		"description": "A list of dynamic links to use with the linker",
		"items": {
			"type": "string"
		}
	})json"_ojson;
	ret[kDefinitions]["target-project-cxx-links"][kItems][kPattern] = patternProjectLinks;

	ret[kDefinitions]["target-project-location"] = R"json({
		"description": "The root path of the source files, relative to the working directory.",
		"oneOf": [
			{
				"type": "string"
			},
			{
				"type": "array",
				"uniqueItems": true,
				"minItems": 1,
				"items": {
					"type": "string"
				}
			},
			{
				"type": "object",
				"additionalProperties": false,
				"required": [
					"include"
				]
			}
		]
	})json"_ojson;
	ret[kDefinitions]["target-project-location"]["oneOf"][2][kPatternProperties][fmt::format("^exclude{}{}$", patternConfigurations, patternPlatforms)] = R"json({
		"anyOf": [
			{
				"type": "string"
			},
			{
				"type": "array",
				"uniqueItems": true,
				"minItems": 1,
				"items": {
					"type": "string"
				}
			}
		]
	})json"_ojson;
	ret[kDefinitions]["target-project-location"]["oneOf"][2][kPatternProperties][fmt::format("^include{}{}$", patternConfigurations, patternPlatforms)] = R"json({
		"anyOf": [
			{
				"type": "string"
			},
			{
				"type": "array",
				"uniqueItems": true,
				"minItems": 1,
				"items": {
					"type": "string"
				}
			}
		]
	})json"_ojson;

	ret[kDefinitions]["target-project-cxx-macosFrameworkPaths"] = R"json({
		"type": "array",
		"description": "A list of paths to search for MacOS Frameworks",
		"uniqueItems": true,
		"minItems": 1,
		"items": {
			"type": "string"
		}
	})json"_ojson;

	ret[kDefinitions]["target-project-cxx-macosFrameworks"] = R"json({
		"type": "array",
		"description": "A list of MacOS Frameworks to link to the project",
		"uniqueItems": true,
		"minItems": 1,
		"items": {
			"type": "string"
		}
	})json"_ojson;

	// ret[kDefinitions]["project-name"] = R"json({
	// 	"type": "string",
	// 	"description": "The name of the project."
	// })json"_ojson;
	// ret[kDefinitions]["project-name"][kPattern] = patternProjectName;

	ret[kDefinitions]["target-project-cxx-objectiveCxx"] = R"json({
		"type": "boolean",
		"description": "Set to true if compiling Objective-C or Objective-C++ files (.m or .mm), or including any Objective-C/C++ headers.",
		"default": false
	})json"_ojson;

	ret[kDefinitions]["target-project-cxx-pch"] = R"json({
		"type": "string",
		"description": "Compile a header file as a pre-compiled header and include it in compilation of every object file in the project. Define a path relative to the workspace root."
	})json"_ojson;

	ret[kDefinitions]["target-project-cxx-threads"] = R"json({
		"type": "string",
		"enum": [
			"auto",
			"posix",
			"none"
		],
		"default": "auto"
	})json"_ojson;

	ret[kDefinitions]["target-project-cxx-rtti"] = R"json({
		"type": "boolean",
		"description": "true to include run-time type information (default), false to exclude.",
		"default": true
	})json"_ojson;

	ret[kDefinitions]["target-project-runProject"] = R"json({
		"type": "boolean",
		"description": "Is this the main project to run during run-related commands (buildrun & run)?\n\nIf multiple targets are defined as true, the first will be chosen to run. If a command-line runProject is given, it will be prioritized.",
		"default": false
	})json"_ojson;

	ret[kDefinitions]["target-project-runArguments"] = R"json({
		"type": "array",
		"description": "If the project is the run target, a string of arguments to pass to the run command.",
		"minItems": 1,
		"items": {
			"type": "string"
		}
	})json"_ojson;

	ret[kDefinitions]["target-project-runDependencies"] = R"json({
		"type": "array",
		"uniqueItems": true,
		"description": "If the project is the run target, a list of dynamic libraries that should be copied before running.",
		"minItems": 1,
		"items": {
			"type": "string"
		}
	})json"_ojson;

	ret[kDefinitions]["target-script-script"] = R"json({
		"anyOf": [
			{
				"type": "string"
			},
			{
				"type": "array",
				"uniqueItems": true,
				"minItems": 1,
				"items": {
					"type": "string"
				}
			}
		]
	})json"_ojson;

	ret[kDefinitions]["target-project-cxx-staticLinking"] = R"json({
		"description": "true to statically link against compiler libraries (libc++, etc.). false to dynamically link them.",
		"type": "boolean",
		"default": false
	})json"_ojson;

	ret[kDefinitions]["target-project-cxx-staticLinks"] = R"json({
		"type": "array",
		"description": "A list of static links to use with the linker",
		"uniqueItems": true,
		"minItems": 1,
		"items": {
			"type": "string"
		}
	})json"_ojson;
	ret[kDefinitions]["target-project-cxx-staticLinks"][kItems][kPattern] = patternProjectLinks;

	ret[kDefinitions]["target-project-cxx-warnings"] = R"json({
		"description": "Either a preset of the warnings to use, or the warnings flags themselves (excluding '-W' prefix)",
		"anyOf": [
			{
				"type": "string",
				"enum": [
					"none",
					"minimal",
					"error",
					"pedantic",
					"strict",
					"strictPedantic",
					"veryStrict"
				]
			},
			{
				"type": "array",
				"items": {
					"type": "string",
					"uniqueItems": true,
					"minItems": 1
				}
			}
		]
	})json"_ojson;

	ret[kDefinitions]["target-project-cxx-warnings"][kAnyOf][1][kItems][kEnum] = {
		"abi",
		"absolute-value",
		"address",
		"aggregate-return",
		"all",
		"alloc-size-larger-than=CC_ALLOC_SIZE_LARGER_THAN",
		"alloc-zero",
		"alloca",
		"alloca-larger-than=CC_ALLOCA_LARGER_THAN",
		"arith-conversion",
		"array-bounds",
		"array-bounds=1",
		"array-bounds=2",
		"array-parameter",
		"array-parameter=1",
		"array-parameter=2",
		"attribute-alias",
		"attribute-alias=0",
		"attribute-alias=1",
		"attribute-alias=2",
		"bad-function-cast",
		"bool-compare",
		"bool-operation",
		"c90-c99-compat",
		"c99-c11-compat",
		"c11-c2x-compat",
		"c++-compat",
		"c++11-compat",
		"c++14-compat",
		"c++17-compat",
		"c++20-compat",
		"cast-align",
		"cast-align=strict",
		"cast-function-type",
		"cast-qual",
		"catch-value",
		"char-subscripts",
		"clobbered",
		"comment",
		"comments",
		"conversion",
		"dangling-else",
		"date-time",
		"declaration-after-statement",
		"deprecated-copy",
		"disabled-optimization",
		"double-promotion",
		"duplicate-decl-specifier",
		"duplicated-branches",
		"duplicated-cond",
		"empty-body",
		"enum-compare",
		"enum-conversion",
		"effc++",
		"extra",
		"error",
		"expansion-to-defined",
		"fatal-errors",
		"float-conversion",
		"float-equal",
		"format",
		"format=0",
		"format=1",
		"format=2",
		"format-nonliteral",
		"format-overflow",
		"format-overflow=1",
		"format-overflow=2",
		"format-security",
		"format-signedness",
		"format-truncation",
		"format-truncation=1",
		"format-truncation=2",
		"format-y2k",
		"frame-address",
		"frame-larger-than=CC_FRAME_LARGER_THAN",
		"ignored-qualifiers",
		"implicit-fallthrough",
		"implicit-fallthrough=0",
		"implicit-fallthrough=1",
		"implicit-fallthrough=2",
		"implicit-fallthrough=3",
		"implicit-fallthrough=4",
		"implicit-fallthrough=5",
		"implicit",
		"implicit-int",
		"implicit-function-declaration",
		"init-self",
		"inline",
		"int-in-bool-context",
		"invalid-memory-model",
		"invalid-pch",
		"jump-misses-init",
		"larger-than=CC_LARGER_THAN",
		"logical-not-parentheses",
		"logical-op",
		"long-long",
		"main",
		"maybe-uninitialized",
		"memset-elt-size",
		"memset-transposed-args",
		"misleading-indentation",
		"missing-attributes",
		"missing-braces",
		"missing-declarations",
		"missing-field-initializers",
		"missing-include-dirs",
		"missing-parameter-type",
		"missing-prototypes",
		"multistatement-macros",
		"narrowing",
		"nested-externs",
		"no-address-of-packed-member",
		"no-aggressive-loop-optimizations",
		"no-alloc-size-larger-than",
		"no-alloca-larger-than",
		"no-attribute-alias",
		"no-attribute-warning",
		"no-attributes",
		"no-builtin-declaration-mismatch",
		"no-builtin-macro-redefined",
		"no-coverate-mismatch",
		"no-cpp",
		"no-deprecated",
		"no-deprecated-declarations",
		"no-designated-init",
		"no-discarded-qualifier",
		"no-discarded-array-qualifiers",
		"no-div-by-zero",
		"no-endif-labels",
		"no-incompatible-pointer-types",
		"no-int-conversion",
		"no-format-contains-nul",
		"no-format-extra-args",
		"no-format-zero-length",
		"no-frame-larger-than",
		"no-free-nonheap-object",
		"no-if-not-aligned",
		"no-ignored-attributes",
		"no-implicit-int",
		"no-implicit-function-declaration",
		"no-int-to-pointer-cast",
		"no-invalid-memory-model",
		"no-larger-than",
		"no-long-long",
		"no-lto-type-mismatch",
		"no-missing-profile",
		"no-missing-field-initializers",
		"no-multichar",
		"no-odr",
		"no-overflow",
		"no-overlength-strings",
		"no-override-init-side-effects",
		"no-pedantic-ms-format",
		"no-pointer-compare",
		"no-pointer-to-int-cast",
		"no-pragmas",
		"no-prio-ctor-dtor",
		"no-return-local-addr",
		"no-scalar-storage-order",
		"no-shadow-ivar",
		"no-shift-count-negative",
		"no-shift-count-overflow",
		"no-shift-overflow",
		"no-sizeof-array-argument",
		"no-stack-usage",
		"no-stringop-overflow",
		"no-stringop-overread",
		"no-stringop-truncation",
		"no-switch-bool",
		"no-switch-outside-range",
		"no-switch-unreachable",
		"no-trigraphs",
		"no-unused-function",
		"no-unused-result",
		"no-unused-variable",
		"no-varargs",
		"no-variadic-macros",
		"no-vla",
		"no-vla-larger-than",
		"noexcept",
		"non-virtual-dtor",
		"nonnull",
		"nonnull-compare",
		"nopacked-bitfield-compat",
		"normalized=none",
		"normalized=id",
		"normalized=nfc",
		"normalized=nfkc",
		"null-dereference",
		"odr",
		"old-style-cast",
		"old-style-declaration",
		"old-style-definition",
		"openmp-simd",
		"overlength-strings",
		"overloaded-virtual",
		"override-init",
		"packed",
		"packed-not-aligned",
		"padded",
		"parentheses",
		"pedantic",
		"pedantic-errors",
		"pessimizing-move",
		"pointer-arith",
		"pointer-sign",
		"range-loop-construct",
		"redundant-decls",
		"redundant-move",
		"reorder",
		"restrict",
		"return-type",
		"scrict-null-sentinel",
		"sequence-point",
		"shadow",
		"shadow=global",
		"shadow=local",
		"shadow=compatible-local",
		"shift-negative-value",
		"shift-overflow=1",
		"shift-overflow=2",
		"sign-compare",
		"sign-conversion",
		"sign-promo",
		"sizeof-array-div",
		"sizeof-pointer-div",
		"sizeof-pointer-memaccess",
		"stack-protector",
		"stack-usage=CC_STACK_USAGE",
		"strict-aliasing",
		"strict-aliasing=1",
		"strict-aliasing=2",
		"strict-aliasing=3",
		"strict-overflow",
		"strict-overflow=1",
		"strict-overflow=2",
		"strict-overflow=3",
		"strict-overflow=4",
		"strict-overflow=5",
		"strict-prototypes",
		"string-compare",
		"stringop-overflow",
		"stringop-overflow=1",
		"stringop-overflow=2",
		"stringop-overflow=3",
		"stringop-overflow=4",
		"suggest-attribute=pure",
		"suggest-attribute=const",
		"suggest-attribute=noreturn",
		"suggest-attribute=format",
		"suggest-attribute=cold",
		"suggest-attribute=malloc",
		"switch",
		"switch-default",
		"switch-enum",
		"switch-unreachable",
		"sync-nand",
		"system-headers",
		"tautological-compare",
		"traditional",
		"traditional-conversion",
		"trampolines",
		"trigraphs",
		"type-limits",
		"undef",
		"uninitialized",
		"unknown-pragmas",
		"unreachable-code",
		"unsafe-loop-optimizations",
		"unsuffixed-float-constants",
		"unused",
		"unused-but-set-parameter",
		"unused-but-set-variable",
		"unused-const-variable",
		"unused-const-variable=1",
		"unused-const-variable=2",
		"unused-function",
		"unused-label",
		"unused-local-typedefs",
		"unused-macros",
		"unused-parameter",
		"unused-value",
		"unused-variable",
		"variadic-macros",
		"vector-operation-performance",
		"vla",
		"vla-larger-than=CC_VLA_LARGER_THAN",
		"vla-parameter",
		"volatile-register-var",
		"write-strings",
		"zero-as-null-pointer-constant",
		"zero-length-bounds"
	};

	ret[kDefinitions]["target-project-cxx-windowsApplicationManifest"] = R"json({
		"description": "The path to a Windows application manifest. Only applies to application (kind=[consoleApplication|desktopApplication]) and shared library (kind=sharedLibrary) targets",
		"type": "string"
	})json"_ojson;

	ret[kDefinitions]["target-project-cxx-windowsApplicationIcon"] = R"json({
		"type": "string",
		"description": "The windows icon to use for the project. Only applies to application targets (kind=[consoleApplication|desktopApplication])"
	})json"_ojson;

	ret[kDefinitions]["target-project-cxx-windowsOutputDef"] = R"json({
		"type": "boolean",
		"description": "If true for a shared library (kind=sharedLibrary) target on Windows, a .def file will be created",
		"default": false
	})json"_ojson;

	ret[kDefinitions]["target-project-cxx-windowsPrefixOutputFilename"] = R"json({
		"type": "boolean",
		"description": "Only applies to shared library targets (kind=sharedLibrary) on windows. If true, prefixes the output dll with 'lib'. This may not be desirable with standalone dlls.",
		"default": true
	})json"_ojson;

	ret[kDefinitions]["target-script"] = R"json({
		"type": "object",
		"additionalProperties": false,
		"properties": {
			"script": {
				"description": "Script(s) to run during this build step.",
				"$ref": "#/definitions/target-script-script"
			},
			"description": {
				"$ref": "#/definitions/target-description"
			}
		}
	})json"_ojson;
	ret[kDefinitions]["target-script"][kPatternProperties][fmt::format("^script{}{}$", patternConfigurations, patternPlatforms)] = R"json({
		"description": "Script(s) to run during this build step.",
		"$ref": "#/definitions/target-script-script"
	})json"_ojson;
	ret[kDefinitions]["target-script"][kPatternProperties][fmt::format("^description{}{}$", patternConfigurations, patternPlatforms)] = R"json({
		"$ref": "#/definitions/target-description"
	})json"_ojson;

	ret[kDefinitions]["target-type"] = R"json({
		"type": "string",
		"description": "The target type, if not a local project or script.",
		"enum": ["CMake", "Chalet"]
	})json"_ojson;

	ret[kDefinitions]["target-cmake-location"] = R"json({
		"type": "string",
		"description": "The folder path of the root CMakeLists.txt for the project."
	})json"_ojson;

	ret[kDefinitions]["target-cmake-buildFile"] = R"json({
		"type": "string",
		"description": "The build file to use, if not CMakeLists.txt, relative to the location. (-C)"
	})json"_ojson;

	ret[kDefinitions]["target-cmake-defines"] = R"json({
		"type": "array",
		"description": "Macro definitions to be passed into CMake. (-D)",
		"uniqueItems": true,
		"minItems": 1,
		"items": {
			"type": "string"
		}
	})json"_ojson;

	ret[kDefinitions]["target-cmake-recheck"] = R"json({
		"type": "boolean",
		"description": "If true, CMake will be invoked each time during the build.",
		"default": false
	})json"_ojson;

	ret[kDefinitions]["target-cmake-toolset"] = R"json({
		"type": "string",
		"description": "A toolset to be passed to CMake with the -T option."
	})json"_ojson;

	ret[kDefinitions]["target-cmake"] = R"json({
		"type": "object",
		"additionalProperties": false,
		"required": [
			"type",
			"location"
		],
		"description": "Build the location with cmake",
		"properties": {
			"description": {
				"$ref": "#/definitions/target-description"
			},
			"location": {
				"$ref": "#/definitions/target-cmake-location"
			},
			"buildFile": {
				"$ref": "#/definitions/target-cmake-buildFile"
			},
			"defines": {
				"$ref": "#/definitions/target-cmake-defines"
			},
			"toolset": {
				"$ref": "#/definitions/target-cmake-toolset"
			},
			"recheck": {
				"$ref": "#/definitions/target-cmake-recheck"
			},
			"type": {
				"$ref": "#/definitions/target-type"
			},
			"onlyInConfiguration": {
				"$ref": "#/definitions/target-onlyInConfiguration"
			},
			"notInConfiguration": {
				"$ref": "#/definitions/target-notInConfiguration"
			},
			"onlyInPlatform": {
				"$ref": "#/definitions/target-onlyInPlatform"
			},
			"notInPlatform": {
				"$ref": "#/definitions/target-notInPlatform"
			}
		}
	})json"_ojson;
	ret[kDefinitions]["target-cmake"][kPatternProperties][fmt::format("^description{}{}$", patternConfigurations, patternPlatforms)] = R"json({
		"$ref": "#/definitions/target-description"
	})json"_ojson;
	ret[kDefinitions]["target-cmake"][kPatternProperties][fmt::format("^buildFile{}{}$", patternConfigurations, patternPlatforms)] = R"json({
		"$ref": "#/definitions/target-cmake-buildFile"
	})json"_ojson;
	ret[kDefinitions]["target-cmake"][kPatternProperties][fmt::format("^defines{}{}$", patternConfigurations, patternPlatforms)] = R"json({
		"$ref": "#/definitions/target-cmake-defines"
	})json"_ojson;
	ret[kDefinitions]["target-cmake"][kPatternProperties][fmt::format("^toolset{}{}$", patternConfigurations, patternPlatforms)] = R"json({
		"$ref": "#/definitions/target-cmake-toolset"
	})json"_ojson;

	ret[kDefinitions]["target-chalet-location"] = R"json({
		"type": "string",
		"description": "The folder path of the root build.json for the project."
	})json"_ojson;

	ret[kDefinitions]["target-chalet-buildFile"] = R"json({
		"type": "string",
		"description": "The build file to use, if not build.json, relative to the location."
	})json"_ojson;

	ret[kDefinitions]["target-chalet-recheck"] = R"json({
		"type": "boolean",
		"description": "If true, Chalet will be invoked each time during the build."
	})json"_ojson;

	ret[kDefinitions]["target-chalet"] = R"json({
		"type": "object",
		"additionalProperties": false,
		"required": [
			"type",
			"location"
		],
		"description": "Build the location with cmake",
		"properties": {
			"description": {
				"$ref": "#/definitions/target-description"
			},
			"location": {
				"$ref": "#/definitions/target-chalet-location"
			},
			"buildFile": {
				"$ref": "#/definitions/target-chalet-buildFile"
			},
			"recheck": {
				"$ref": "#/definitions/target-chalet-recheck"
			},
			"type": {
				"$ref": "#/definitions/target-type"
			},
			"onlyInConfiguration": {
				"$ref": "#/definitions/target-onlyInConfiguration"
			},
			"notInConfiguration": {
				"$ref": "#/definitions/target-notInConfiguration"
			},
			"onlyInPlatform": {
				"$ref": "#/definitions/target-onlyInPlatform"
			},
			"notInPlatform": {
				"$ref": "#/definitions/target-notInPlatform"
			}
		}
	})json"_ojson;
	ret[kDefinitions]["target-chalet"][kPatternProperties][fmt::format("^description{}{}$", patternConfigurations, patternPlatforms)] = R"json({
		"$ref": "#/definitions/target-description"
	})json"_ojson;
	ret[kDefinitions]["target-cmake"][kPatternProperties][fmt::format("^buildFile{}{}$", patternConfigurations, patternPlatforms)] = R"json({
		"$ref": "#/definitions/target-chalet-buildFile"
	})json"_ojson;

	//
	const auto kProperties = "properties";
	ret[kProperties] = Json::object();
	ret[kPatternProperties] = Json::object();

	ret[kPatternProperties]["^abstracts:[a-z]+$"] = R"json({
		"description": "An abstract build project. 'abstracts:all' is a special project that gets implicitely added to each project",
		"$ref": "#/definitions/target-project"
	})json"_ojson;

	ret[kProperties]["abstracts"] = R"json({
		"type": "object",
		"additionalProperties": false,
		"description": "A list of abstract build projects",
		"patternProperties": {
			"^[A-Za-z_-]+$": {
				"description": "An abstract build project. 'all' is implicitely added to each project.",
				"$ref": "#/definitions/target-project"
			}
		}
	})json"_ojson;

	ret[kProperties]["distribution"] = R"json({
		"type": "object",
		"additionalProperties": false,
		"description": "A list of bundle descriptors for the distribution."
	})json"_ojson;
	ret[kProperties]["distribution"][kPatternProperties][patternDistributionName] = R"json({
		"description": "A single bundle or script.",
		"oneOf": [
			{
				"$ref": "#/definitions/target-script"
			},
			{
				"$ref": "#/definitions/distribution-bundle"
			}
		]
	})json"_ojson;

	ret[kProperties]["configurations"] = R"json({
		"anyOf": [
			{
				"type": "object",
				"additionalProperties": false,
				"description": "A list of allowed build configurations",
				"patternProperties": {
					"^[A-Za-z]{3,}$": {
						"type": "object",
						"additionalProperties": false,
						"properties": {
							"debugSymbols": {
								"$ref": "#/definitions/configurations-debugSymbols"
							},
							"enableProfiling": {
								"$ref": "#/definitions/configurations-enableProfiling"
							},
							"linkTimeOptimization": {
								"$ref": "#/definitions/configurations-linkTimeOptimization"
							},
							"optimizations": {
								"$ref": "#/definitions/configurations-optimizations"
							},
							"stripSymbols": {
								"$ref": "#/definitions/configurations-stripSymbols"
							}
						}
					}
				}
			},
			{
				"type": "array",
				"description": "An array of allowed build configuration presets",
				"uniqueItems": true,
				"minItems": 1,
				"items": {
					"type": "string",
					"description": "A configuration preset",
					"enum": [
						"Release",
						"Debug",
						"RelWithDebInfo",
						"MinSizeRel",
						"Profile"
					]
				}
			}
		]
	})json"_ojson;

	ret[kProperties]["externalDependencies"] = R"json({
		"type": "object",
		"additionalProperties": false,
		"description": "A sequential list of externalDependencies to install prior to building or via the configure command. The key will be the destination directory name for the repository within the folder defined in 'externalDepDir'."
	})json"_ojson;
	ret[kProperties]["externalDependencies"][kPatternProperties]["^[\\w\\-\\+\\.]{3,100}$"] = R"json({
		"description": "A single external dependency.",
		"$ref": "#/definitions/externalDependency"
	})json"_ojson;

	ret[kProperties]["targets"] = R"json({
		"type": "object",
		"additionalProperties": false,
		"description": "A sequential list of projects, cmake projects, or scripts."
	})json"_ojson;
	ret[kProperties]["targets"][kPatternProperties][patternProjectName] = R"json({
		"description": "A single build target or script.",
		"oneOf": [
			{
				"$ref": "#/definitions/target-project"
			},
			{
				"$ref": "#/definitions/target-script"
			},
			{
				"type": "object",
				"properties": {
					"type": {
						"$ref": "#/definitions/target-type"
					}
				},
				 "allOf": [
					{
						"if": {
							"properties": { "type": { "const": "CMake" } }
						},
						"then": {
							"$ref": "#/definitions/target-cmake"
						}
					},
					{
						"if": {
							"properties": { "type": { "const": "Chalet" } }
						},
						"then": {
							"$ref": "#/definitions/target-chalet"
						}
					}
				]
			}
		]
	})json"_ojson;

	ret[kProperties]["version"] = R"json({
		"type": "string",
		"description": "Version of the workspace project.",
		"pattern": "^[\\w\\-\\+\\.]+$"
	})json"_ojson;

	ret[kProperties]["workspace"] = R"json({
		"type": "string",
		"description": "The name of the workspace.",
		"pattern": "^[\\w\\-\\+ ]+$"
	})json"_ojson;

	ret[kProperties]["externalDepDir"] = R"json({
		"type": "string",
		"description": "The path to install external dependencies into (see externalDependencies).",
		"default": "chalet_external"
	})json"_ojson;

	ret[kProperties]["path"] = R"json({
		"$ref": "#/definitions/environment-path"
	})json"_ojson;

	ret[kPatternProperties][fmt::format("^path{}{}$", patternConfigurations, patternPlatforms)] = R"json({
		"$ref": "#/definitions/environment-path"
	})json"_ojson;

	return ret;
}
}
