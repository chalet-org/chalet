/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "BuildJson/SchemaBuildJson.hpp"

#include "FileTemplates/PlatformFileTemplates.hpp"

#include "Utility/String.hpp"
#include "Utility/SuppressIntellisense.hpp"
#include "Json/JsonComments.hpp"

namespace chalet
{
namespace
{
enum class Defs : ushort
{
	ConfigDebugSymbols,
	ConfigEnableProfiling,
	ConfigLinkTimeOptimizations,
	ConfigOptimizationLevel,
	ConfigStripSymbols,
	//
	DistConfiguration,
	DistDependencies,
	DistDescription,
	DistExclude,
	DistIncludeDependentSharedLibs,
	DistLinux,
	DistMacOS,
	DistMainProject,
	DistOutDirectory,
	DistProjects,
	DistWindows,
	//
	ExtGitRepository,
	ExtGitBranch,
	ExtGitCommit,
	ExtGitTag,
	ExtGitSubmodules,
	//
	EnumPlatform,
	//
	EnvironmentPath,
	//
	TargetDescription,
	TargetType,
	TargetNotInConfiguration,
	TargetNotInPlatform,
	TargetOnlyInConfiguration,
	TargetOnlyInPlatform,
	//
	TargetProjectExtends,
	TargetProjectFiles,
	TargetProjectKind,
	TargetProjectLocation,
	TargetProjectLanguage,
	TargetProjectRunProject,
	TargetProjectRunArguments,
	TargetProjectRunDependencies,
	//
	TargetProjectCxxCStandard,
	TargetProjectCxxCppStandard,
	TargetProjectCxxCompileOptions,
	TargetProjectCxxDefines,
	TargetProjectCxxIncludeDirs,
	TargetProjectCxxLibDirs,
	TargetProjectCxxLinkerScript,
	TargetProjectCxxLinkerOptions,
	TargetProjectCxxLinks,
	TargetProjectCxxMacOsFrameworkPaths,
	TargetProjectCxxMacOsFrameworks,
	TargetProjectCxxObjectiveCxx,
	TargetProjectCxxPrecompiledHeader,
	TargetProjectCxxThreads,
	TargetProjectCxxRunTimeTypeInfo,
	TargetProjectCxxExceptions,
	TargetProjectCxxStaticLinking,
	TargetProjectCxxStaticLinks,
	TargetProjectCxxWarnings,
	TargetProjectCxxWindowsAppManifest,
	TargetProjectCxxWindowsAppIcon,
	TargetProjectCxxWindowsOutputDef,
	TargetProjectCxxWindowsPrefixOutputFilename,
	//
	TargetScriptScript,
	//
	TargetCMakeLocation,
	TargetCMakeBuildFile,
	TargetCMakeDefines,
	TargetCMakeRecheck,
	TargetCMakeToolset,
	//
	TargetChaletLocation,
	TargetChaletBuildFile,
	TargetChaletRecheck,
};
}

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
	const auto kProperties = "properties";
	const auto kPattern = "pattern";
	const auto kPatternProperties = "patternProperties";
	const auto kDescription = "description";
	const auto kEnum = "enum";
	const auto kAnyOf = "anyOf";
	const auto kAllOf = "allOf";
	const auto kOneOf = "oneOf";

	std::unordered_map<Defs, Json> defs;

	// configurations
	defs[Defs::ConfigDebugSymbols] = R"json({
		"type": "boolean",
		"description": "true to include debug symbols, false otherwise.",
		"default": false
	})json"_ojson;

	defs[Defs::ConfigEnableProfiling] = R"json({
		"type": "boolean",
		"description": "true to enable profiling for this configuration, false otherwise.",
		"default": false
	})json"_ojson;

	defs[Defs::ConfigLinkTimeOptimizations] = R"json({
		"type": "boolean",
		"description": "true to use link-time optimization, false otherwise.",
		"default": false
	})json"_ojson;

	defs[Defs::ConfigOptimizationLevel] = R"json({
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

	defs[Defs::ConfigStripSymbols] = R"json({
		"type": "boolean",
		"description": "true to strip symbols from the build, false otherwise.",
		"default": false
	})json"_ojson;

	// distribution
	defs[Defs::DistConfiguration] = R"json({
		"type": "string",
		"description": "The name of the build configuration to use for the distribution.",
		"default": "Release"
	})json"_ojson;

	defs[Defs::DistDependencies] = R"json({
		"type": "array",
		"uniqueItems": true,
		"minItems": 1,
		"items": {
			"type": "string"
		}
	})json"_ojson;

	defs[Defs::DistDescription] = R"json({
		"type": "string"
	})json"_ojson;

	defs[Defs::DistExclude] = R"json({
		"type": "array",
		"uniqueItems": true,
		"minItems": 1,
		"items": {
			"type": "string"
		}
	})json"_ojson;

	defs[Defs::DistIncludeDependentSharedLibs] = R"json({
		"type": "boolean",
		"default": true
	})json"_ojson;

	defs[Defs::DistLinux] = R"json({
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

	defs[Defs::DistMacOS] = R"json({
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
	defs[Defs::DistMacOS]["properties"]["infoPropertyList"]["anyOf"][1]["default"] = JsonComments::parseLiteral(PlatformFileTemplates::macosInfoPlist());

	defs[Defs::DistMainProject] = R"json({
		"type": "string",
		"description": "The main executable project."
	})json"_ojson;

	defs[Defs::DistOutDirectory] = R"json({
		"type": "string",
		"description": "The output folder to place the final build along with all of its dependencies.",
		"default": "dist"
	})json"_ojson;

	defs[Defs::DistProjects] = R"json({
		"type": "array",
		"uniqueItems": true,
		"description": "An array of projects to include",
		"minItems": 1,
		"items": {
			"type": "string",
			"description": "The name of the project"
		}
	})json"_ojson;
	defs[Defs::DistProjects][kItems][kPattern] = patternProjectName;

	defs[Defs::DistWindows] = R"json({
		"type": "object",
		"description": "Variables to describe the windows application.",
		"additionalProperties": false,
		"required": [],
		"properties": {
			"nsisScript": {
				"type": "string",
				"description": "Relative path to an NSIS installer script (.nsi) to compile for this distribution target."
			}
		}
	})json"_ojson;

	auto distributionBundle = R"json({
		"type": "object",
		"additionalProperties": false,
		"description": "Variables to describe the final output build."
	})json"_ojson;
	distributionBundle[kProperties] = Json::object();
	distributionBundle[kProperties]["configuration"] = defs[Defs::DistConfiguration];
	distributionBundle[kProperties]["dependencies"] = defs[Defs::DistDependencies];
	distributionBundle[kProperties]["description"] = defs[Defs::DistDescription];
	distributionBundle[kProperties]["exclude"] = defs[Defs::DistExclude];
	distributionBundle[kProperties]["includeDependentSharedLibraries"] = defs[Defs::DistIncludeDependentSharedLibs];
	distributionBundle[kProperties]["linux"] = defs[Defs::DistLinux];
	distributionBundle[kProperties]["macos"] = defs[Defs::DistMacOS];
	distributionBundle[kProperties]["windows"] = defs[Defs::DistWindows];
	distributionBundle[kProperties]["mainProject"] = defs[Defs::DistMainProject];
	distributionBundle[kProperties]["outDir"] = defs[Defs::DistOutDirectory];
	distributionBundle[kProperties]["projects"] = defs[Defs::DistProjects];
	distributionBundle[kPatternProperties][fmt::format("^dependencies{}{}$", patternConfigurations, patternPlatforms)] = defs[Defs::DistDependencies];
	distributionBundle[kPatternProperties][fmt::format("^exclude{}{}$", patternConfigurations, patternPlatforms)] = defs[Defs::DistExclude];

	// externalDependency
	defs[Defs::ExtGitRepository] = R"json({
		"type": "string",
		"description": "The url of the git repository.",
		"pattern": "^(?:git|ssh|https?|git@[-\\w.]+):(\\/\\/)?(.*?)(\\.git)(\\/?|\\#[-\\d\\w._]+?)$"
	})json"_ojson;

	defs[Defs::ExtGitBranch] = R"json({
		"type": "string",
		"description": "The branch to checkout. Uses the repository's default if not set."
	})json"_ojson;

	defs[Defs::ExtGitCommit] = R"json({
		"type": "string",
		"description": "The SHA1 hash of the commit to checkout.",
		"pattern": "^[0-9a-f]{7,40}$"
	})json"_ojson;

	defs[Defs::ExtGitTag] = R"json({
		"type": "string",
		"description": "The tag to checkout on the selected branch. If it's blank or not found, the head of the branch will be checked out."
	})json"_ojson;

	defs[Defs::ExtGitSubmodules] = R"json({
		"type": "boolean",
		"description": "Do submodules need to be cloned?",
		"default": false
	})json"_ojson;

	auto externalDependency = R"json({
		"type": "object",
		"oneOf": [
			{
				"additionalProperties": false,
				"required": [
					"repository",
					"tag"
				]
			},
			{
				"additionalProperties": false,
				"required": [
					"repository"
				]
			}
		]
	})json"_ojson;
	externalDependency[kOneOf][0][kProperties] = Json::object();
	externalDependency[kOneOf][0][kProperties]["repository"] = defs[Defs::ExtGitRepository];
	externalDependency[kOneOf][0][kProperties]["submodules"] = defs[Defs::ExtGitSubmodules];
	externalDependency[kOneOf][0][kProperties]["tag"] = defs[Defs::ExtGitTag];

	externalDependency[kOneOf][1][kProperties] = Json::object();
	externalDependency[kOneOf][1][kProperties]["repository"] = defs[Defs::ExtGitRepository];
	externalDependency[kOneOf][1][kProperties]["submodules"] = defs[Defs::ExtGitSubmodules];
	externalDependency[kOneOf][1][kProperties]["branch"] = defs[Defs::ExtGitBranch];
	externalDependency[kOneOf][1][kProperties]["commit"] = defs[Defs::ExtGitCommit];

	defs[Defs::EnumPlatform] = R"json({
		"type": "string",
		"enum": [
			"windows",
			"macos",
			"linux"
		]
	})json"_ojson;

	defs[Defs::EnvironmentPath] = R"json({
		"type": "array",
		"description": "Any additional paths to include.",
		"uniqueItems": true,
		"minItems": 1,
		"items": {
			"type": "string"
		}
	})json"_ojson;

	defs[Defs::TargetDescription] = R"json({
		"type": "string",
		"description": "A description of the target to display during the build."
	})json"_ojson;

	defs[Defs::TargetNotInConfiguration] = R"json({
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

	defs[Defs::TargetNotInPlatform] = R"json({
		"description": "Don't compile this project on specific platform(s)",
		"oneOf": [
			{
				"type": "object"
			},
			{
				"type": "array",
				"uniqueItems": true,
				"minItems": 1
			}
		]
	})json"_ojson;
	defs[Defs::TargetNotInPlatform][kOneOf][0] = defs[Defs::EnumPlatform];
	defs[Defs::TargetNotInPlatform][kOneOf][1][kItems] = defs[Defs::EnumPlatform];

	defs[Defs::TargetOnlyInConfiguration] = R"json({
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

	defs[Defs::TargetOnlyInPlatform] = R"json({
		"description": "Only compile this project on specific platform(s)",
		"oneOf": [
			{
				"type": "object"
			},
			{
				"type": "array",
				"uniqueItems": true,
				"minItems": 1
			}
		]
	})json"_ojson;
	defs[Defs::TargetOnlyInPlatform][kOneOf][0] = defs[Defs::EnumPlatform];
	defs[Defs::TargetOnlyInPlatform][kOneOf][1][kItems] = defs[Defs::EnumPlatform];

	defs[Defs::TargetProjectExtends] = R"json({
		"type": "string",
		"description": "A project template to extend. Defaults to 'all' implicitly.",
		"pattern": "^[A-Za-z_-]+$",
		"default": "all"
	})json"_ojson;

	defs[Defs::TargetProjectFiles] = R"json({
		"type": "array",
		"uniqueItems": true,
		"minItems": 1,
		"description": "Explicitly define the source files, relative to the working directory.",
		"items": {
			"type": "string"
		}
	})json"_ojson;

	defs[Defs::TargetProjectKind] = R"json({
		"type": "string",
		"description": "The type of the project's compiled binary.",
		"enum": [
			"staticLibrary",
			"sharedLibrary",
			"consoleApplication",
			"desktopApplication"
		]
	})json"_ojson;

	defs[Defs::TargetProjectLanguage] = R"json({
		"type": "string",
		"description": "The target language of the project.",
		"enum": [
			"C",
			"C++"
		],
		"default": "C++"
	})json"_ojson;

	defs[Defs::TargetProjectRunProject] = R"json({
		"type": "boolean",
		"description": "Is this the main project to run during run-related commands (buildrun & run)?\n\nIf multiple targets are defined as true, the first will be chosen to run. If a command-line runProject is given, it will be prioritized.",
		"default": false
	})json"_ojson;

	defs[Defs::TargetProjectRunArguments] = R"json({
		"type": "array",
		"description": "If the project is the run target, a string of arguments to pass to the run command.",
		"minItems": 1,
		"items": {
			"type": "string"
		}
	})json"_ojson;

	defs[Defs::TargetProjectRunDependencies] = R"json({
		"type": "array",
		"uniqueItems": true,
		"description": "If the project is the run target, a list of dynamic libraries that should be copied before running.",
		"minItems": 1,
		"items": {
			"type": "string"
		}
	})json"_ojson;

	defs[Defs::TargetProjectCxxCStandard] = R"json({
		"type": "string",
		"description": "The C standard to use in the compilation",
		"pattern": "^((c|gnu)\\d[\\dx]|(iso9899:(1990|199409|1999|199x|20\\d{2})))$",
		"default": "c11"
	})json"_ojson;

	defs[Defs::TargetProjectCxxCompileOptions] = R"json({
		"type": "array",
		"uniqueItems": true,
		"minItems": 1,
		"description": "Options to add during the compilation step.",
		"items": {
			"type": "string"
		}
	})json"_ojson;

	defs[Defs::TargetProjectCxxCppStandard] = R"json({
		"type": "string",
		"description": "The C++ standard to use in the compilation",
		"pattern": "^(c|gnu)\\+\\+\\d[\\dxyzab]$",
		"default": "c++17"
	})json"_ojson;

	defs[Defs::TargetProjectCxxDefines] = R"json({
		"type": "array",
		"uniqueItems": true,
		"minItems": 1,
		"description": "Macro definitions to be used by the preprocessor",
		"items": {
			"type": "string"
		}
	})json"_ojson;

	defs[Defs::TargetProjectCxxIncludeDirs] = R"json({
		"type": "array",
		"uniqueItems": true,
		"minItems": 1,
		"description": "A list of directories to include with the project.",
		"items": {
			"type": "string"
		}
	})json"_ojson;

	defs[Defs::TargetProjectCxxLibDirs] = R"json({
		"type": "array",
		"uniqueItems": true,
		"minItems": 1,
		"description": "Fallback search paths to look for static or dynamic libraries (/usr/lib is included by default)",
		"items": {
			"type": "string"
		}
	})json"_ojson;

	defs[Defs::TargetProjectCxxLinkerScript] = R"json({
		"type": "string",
		"description": "An LD linker script path (.ld file) to pass to the linker command"
	})json"_ojson;

	defs[Defs::TargetProjectCxxLinkerOptions] = R"json({
		"type": "array",
		"uniqueItems": true,
		"minItems": 1,
		"description": "Options to add during the linking step.",
		"items": {
			"type": "string"
		}
	})json"_ojson;

	defs[Defs::TargetProjectCxxLinks] = R"json({
		"type": "array",
		"uniqueItems": true,
		"minItems": 1,
		"description": "A list of dynamic links to use with the linker",
		"items": {
			"type": "string"
		}
	})json"_ojson;
	defs[Defs::TargetProjectCxxLinks][kItems][kPattern] = patternProjectLinks;

	defs[Defs::TargetProjectLocation] = R"json({
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
	defs[Defs::TargetProjectLocation][kOneOf][2][kPatternProperties][fmt::format("^exclude{}{}$", patternConfigurations, patternPlatforms)] = R"json({
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
	defs[Defs::TargetProjectLocation][kOneOf][2][kPatternProperties][fmt::format("^include{}{}$", patternConfigurations, patternPlatforms)] = R"json({
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

	defs[Defs::TargetProjectCxxMacOsFrameworkPaths] = R"json({
		"type": "array",
		"description": "A list of paths to search for MacOS Frameworks",
		"uniqueItems": true,
		"minItems": 1,
		"items": {
			"type": "string"
		}
	})json"_ojson;

	defs[Defs::TargetProjectCxxMacOsFrameworks] = R"json({
		"type": "array",
		"description": "A list of MacOS Frameworks to link to the project",
		"uniqueItems": true,
		"minItems": 1,
		"items": {
			"type": "string"
		}
	})json"_ojson;

	defs[Defs::TargetProjectCxxObjectiveCxx] = R"json({
		"type": "boolean",
		"description": "Set to true if compiling Objective-C or Objective-C++ files (.m or .mm), or including any Objective-C/C++ headers.",
		"default": false
	})json"_ojson;

	defs[Defs::TargetProjectCxxPrecompiledHeader] = R"json({
		"type": "string",
		"description": "Compile a header file as a pre-compiled header and include it in compilation of every object file in the project. Define a path relative to the workspace root."
	})json"_ojson;

	defs[Defs::TargetProjectCxxThreads] = R"json({
		"type": "string",
		"enum": [
			"auto",
			"posix",
			"none"
		],
		"default": "auto"
	})json"_ojson;

	defs[Defs::TargetProjectCxxRunTimeTypeInfo] = R"json({
		"type": "boolean",
		"description": "true to include run-time type information (default), false to exclude.",
		"default": true
	})json"_ojson;

	defs[Defs::TargetProjectCxxExceptions] = R"json({
		"type": "boolean",
		"description": "true to use exceptions (default), false to turn off exceptions.",
		"default": true
	})json"_ojson;

	defs[Defs::TargetProjectCxxStaticLinking] = R"json({
		"description": "true to statically link against compiler libraries (libc++, etc.). false to dynamically link them.",
		"type": "boolean",
		"default": false
	})json"_ojson;

	defs[Defs::TargetProjectCxxStaticLinks] = R"json({
		"type": "array",
		"description": "A list of static links to use with the linker",
		"uniqueItems": true,
		"minItems": 1,
		"items": {
			"type": "string"
		}
	})json"_ojson;
	defs[Defs::TargetProjectCxxStaticLinks][kItems][kPattern] = patternProjectLinks;

	defs[Defs::TargetProjectCxxWarnings] = R"json({
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

	defs[Defs::TargetProjectCxxWarnings][kAnyOf][1][kItems][kEnum] = {
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

	defs[Defs::TargetProjectCxxWindowsAppManifest] = R"json({
		"description": "The path to a Windows application manifest. Only applies to application (kind=[consoleApplication|desktopApplication]) and shared library (kind=sharedLibrary) targets",
		"type": "string"
	})json"_ojson;

	defs[Defs::TargetProjectCxxWindowsAppIcon] = R"json({
		"type": "string",
		"description": "The windows icon to use for the project. Only applies to application targets (kind=[consoleApplication|desktopApplication])"
	})json"_ojson;

	defs[Defs::TargetProjectCxxWindowsOutputDef] = R"json({
		"type": "boolean",
		"description": "If true for a shared library (kind=sharedLibrary) target on Windows, a .def file will be created",
		"default": false
	})json"_ojson;

	defs[Defs::TargetProjectCxxWindowsPrefixOutputFilename] = R"json({
		"type": "boolean",
		"description": "Only applies to shared library targets (kind=sharedLibrary) on windows. If true, prefixes the output dll with 'lib'. This may not be desirable with standalone dlls.",
		"default": true
	})json"_ojson;

	auto projectSettingsCxx = R"json({
		"type": "object",
		"additionalProperties": false
	})json"_ojson;
	projectSettingsCxx[kProperties]["cStandard"] = defs[Defs::TargetProjectCxxCStandard];
	projectSettingsCxx[kProperties]["compileOptions"] = defs[Defs::TargetProjectCxxCompileOptions];
	projectSettingsCxx[kProperties]["cppStandard"] = defs[Defs::TargetProjectCxxCppStandard];
	projectSettingsCxx[kProperties]["defines"] = defs[Defs::TargetProjectCxxDefines];
	projectSettingsCxx[kProperties]["includeDirs"] = defs[Defs::TargetProjectCxxIncludeDirs];
	projectSettingsCxx[kProperties]["libDirs"] = defs[Defs::TargetProjectCxxLibDirs];
	projectSettingsCxx[kProperties]["linkerScript"] = defs[Defs::TargetProjectCxxLinkerScript];
	projectSettingsCxx[kProperties]["linkerOptions"] = defs[Defs::TargetProjectCxxLinkerOptions];
	projectSettingsCxx[kProperties]["links"] = defs[Defs::TargetProjectCxxLinks];
	projectSettingsCxx[kProperties]["macosFrameworkPaths"] = defs[Defs::TargetProjectCxxMacOsFrameworkPaths];

	projectSettingsCxx[kProperties]["macosFrameworks"] = defs[Defs::TargetProjectCxxMacOsFrameworks];
	projectSettingsCxx[kProperties]["objectiveCxx"] = defs[Defs::TargetProjectCxxObjectiveCxx];
	projectSettingsCxx[kProperties]["pch"] = defs[Defs::TargetProjectCxxPrecompiledHeader];
	projectSettingsCxx[kProperties]["threads"] = defs[Defs::TargetProjectCxxThreads];
	projectSettingsCxx[kProperties]["rtti"] = defs[Defs::TargetProjectCxxRunTimeTypeInfo];
	projectSettingsCxx[kProperties]["exceptions"] = defs[Defs::TargetProjectCxxExceptions];
	projectSettingsCxx[kProperties]["staticLinking"] = defs[Defs::TargetProjectCxxStaticLinking];
	projectSettingsCxx[kProperties]["staticLinks"] = defs[Defs::TargetProjectCxxStaticLinks];
	projectSettingsCxx[kProperties]["warnings"] = defs[Defs::TargetProjectCxxWarnings];
	projectSettingsCxx[kProperties]["windowsPrefixOutputFilename"] = defs[Defs::TargetProjectCxxWindowsPrefixOutputFilename];
	projectSettingsCxx[kProperties]["windowsOutputDef"] = defs[Defs::TargetProjectCxxWindowsOutputDef];
	projectSettingsCxx[kProperties]["windowsApplicationIcon"] = defs[Defs::TargetProjectCxxWindowsAppIcon];
	projectSettingsCxx[kProperties]["windowsApplicationManifest"] = defs[Defs::TargetProjectCxxWindowsAppManifest];

	projectSettingsCxx[kPatternProperties][fmt::format("^cStandard{}$", patternPlatforms)] = defs[Defs::TargetProjectCxxCStandard];
	projectSettingsCxx[kPatternProperties][fmt::format("^cppStandard{}$", patternPlatforms)] = defs[Defs::TargetProjectCxxCppStandard];
	projectSettingsCxx[kPatternProperties][fmt::format("^compileOptions{}$", patternPlatforms)] = defs[Defs::TargetProjectCxxCompileOptions];
	projectSettingsCxx[kPatternProperties][fmt::format("^defines{}{}$", patternConfigurations, patternPlatforms)] = defs[Defs::TargetProjectCxxDefines];
	projectSettingsCxx[kPatternProperties][fmt::format("^includeDirs{}$", patternPlatforms)] = defs[Defs::TargetProjectCxxIncludeDirs];
	projectSettingsCxx[kPatternProperties][fmt::format("^libDirs{}$", patternPlatforms)] = defs[Defs::TargetProjectCxxLibDirs];
	projectSettingsCxx[kPatternProperties][fmt::format("^linkerScript{}$", patternPlatforms)] = defs[Defs::TargetProjectCxxLinkerScript];
	projectSettingsCxx[kPatternProperties][fmt::format("^linkerOptions{}$", patternPlatforms)] = defs[Defs::TargetProjectCxxLinkerOptions];
	projectSettingsCxx[kPatternProperties][fmt::format("^links{}{}$", patternConfigurations, patternPlatforms)] = defs[Defs::TargetProjectCxxLinks];
	projectSettingsCxx[kPatternProperties][fmt::format("^objectiveCxx{}$", patternPlatforms)] = defs[Defs::TargetProjectCxxObjectiveCxx];
	projectSettingsCxx[kPatternProperties][fmt::format("^staticLinks{}{}$", patternConfigurations, patternPlatforms)] = defs[Defs::TargetProjectCxxStaticLinks];
	projectSettingsCxx[kPatternProperties][fmt::format("^threads{}{}$", patternConfigurations, patternPlatforms)] = defs[Defs::TargetProjectCxxThreads];
	projectSettingsCxx[kPatternProperties][fmt::format("^rtti{}{}$", patternConfigurations, patternPlatforms)] = defs[Defs::TargetProjectCxxRunTimeTypeInfo];
	projectSettingsCxx[kPatternProperties][fmt::format("^exceptions{}{}$", patternConfigurations, patternPlatforms)] = defs[Defs::TargetProjectCxxExceptions];
	projectSettingsCxx[kPatternProperties][fmt::format("^staticLinking{}{}$", patternConfigurations, patternPlatforms)] = defs[Defs::TargetProjectCxxStaticLinking];

	auto targetProject = R"json({
		"type": "object",
		"additionalProperties": false
	})json"_ojson;
	targetProject[kProperties]["settings:Cxx"] = std::move(projectSettingsCxx);
	targetProject[kProperties]["extends"] = defs[Defs::TargetProjectExtends];
	targetProject[kProperties]["files"] = defs[Defs::TargetProjectFiles];
	targetProject[kProperties]["kind"] = defs[Defs::TargetProjectKind];
	targetProject[kProperties]["language"] = defs[Defs::TargetProjectLanguage];
	targetProject[kProperties]["location"] = defs[Defs::TargetProjectLocation];
	targetProject[kProperties]["onlyInConfiguration"] = defs[Defs::TargetOnlyInConfiguration];
	targetProject[kProperties]["notInConfiguration"] = defs[Defs::TargetNotInConfiguration];
	targetProject[kProperties]["onlyInPlatform"] = defs[Defs::TargetOnlyInPlatform];
	targetProject[kProperties]["notInPlatform"] = defs[Defs::TargetNotInPlatform];
	targetProject[kProperties]["runProject"] = defs[Defs::TargetProjectRunProject];
	targetProject[kProperties]["runArguments"] = defs[Defs::TargetProjectRunArguments];
	targetProject[kProperties]["runDependencies"] = defs[Defs::TargetProjectRunDependencies];
	targetProject[kPatternProperties][fmt::format("^runProject{}{}$", patternConfigurations, patternPlatforms)] = defs[Defs::TargetProjectRunProject];
	targetProject[kPatternProperties][fmt::format("^runDependencies{}{}$", patternConfigurations, patternPlatforms)] = defs[Defs::TargetProjectRunDependencies];

	defs[Defs::TargetScriptScript] = R"json({
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

	auto targetScript = R"json({
		"type": "object",
		"additionalProperties": false
	})json"_ojson;
	targetScript[kProperties]["script"] = defs[Defs::TargetScriptScript];
	targetScript[kProperties]["script"][kDescription] = "Script(s) to run during this build step.";
	targetScript[kProperties]["description"] = defs[Defs::TargetDescription];
	{
		auto scriptPattern = fmt::format("^script{}{}$", patternConfigurations, patternPlatforms);
		targetScript[kPatternProperties][scriptPattern] = defs[Defs::TargetScriptScript];
		targetScript[kPatternProperties][scriptPattern][kDescription] = "Script(s) to run during this build step.";
	}
	targetScript[kPatternProperties][fmt::format("^description{}{}$", patternConfigurations, patternPlatforms)] = defs[Defs::TargetDescription];

	defs[Defs::TargetType] = R"json({
		"type": "string",
		"description": "The target type, if not a local project or script.",
		"enum": ["CMake", "Chalet"]
	})json"_ojson;

	defs[Defs::TargetCMakeLocation] = R"json({
		"type": "string",
		"description": "The folder path of the root CMakeLists.txt for the project."
	})json"_ojson;

	defs[Defs::TargetCMakeBuildFile] = R"json({
		"type": "string",
		"description": "The build file to use, if not CMakeLists.txt, relative to the location. (-C)"
	})json"_ojson;

	defs[Defs::TargetCMakeDefines] = R"json({
		"type": "array",
		"description": "Macro definitions to be passed into CMake. (-D)",
		"uniqueItems": true,
		"minItems": 1,
		"items": {
			"type": "string"
		}
	})json"_ojson;

	defs[Defs::TargetCMakeRecheck] = R"json({
		"type": "boolean",
		"description": "If true, CMake will be invoked each time during the build.",
		"default": false
	})json"_ojson;

	defs[Defs::TargetCMakeToolset] = R"json({
		"type": "string",
		"description": "A toolset to be passed to CMake with the -T option."
	})json"_ojson;

	auto targetCMake = R"json({
		"type": "object",
		"additionalProperties": false,
		"required": [
			"type",
			"location"
		],
		"description": "Build the location with cmake"
	})json"_ojson;
	targetCMake[kProperties]["description"] = defs[Defs::TargetDescription];
	targetCMake[kProperties]["location"] = defs[Defs::TargetCMakeLocation];
	targetCMake[kProperties]["buildFile"] = defs[Defs::TargetCMakeBuildFile];
	targetCMake[kProperties]["defines"] = defs[Defs::TargetCMakeDefines];
	targetCMake[kProperties]["toolset"] = defs[Defs::TargetCMakeToolset];
	targetCMake[kProperties]["recheck"] = defs[Defs::TargetCMakeRecheck];
	targetCMake[kProperties]["type"] = defs[Defs::TargetType];
	targetCMake[kProperties]["onlyInConfiguration"] = defs[Defs::TargetOnlyInConfiguration];
	targetCMake[kProperties]["notInConfiguration"] = defs[Defs::TargetNotInConfiguration];
	targetCMake[kProperties]["onlyInPlatform"] = defs[Defs::TargetOnlyInPlatform];
	targetCMake[kProperties]["notInPlatform"] = defs[Defs::TargetNotInPlatform];
	targetCMake[kPatternProperties][fmt::format("^description{}{}$", patternConfigurations, patternPlatforms)] = defs[Defs::TargetDescription];
	targetCMake[kPatternProperties][fmt::format("^buildFile{}{}$", patternConfigurations, patternPlatforms)] = defs[Defs::TargetCMakeBuildFile];
	targetCMake[kPatternProperties][fmt::format("^defines{}{}$", patternConfigurations, patternPlatforms)] = defs[Defs::TargetCMakeDefines];
	targetCMake[kPatternProperties][fmt::format("^toolset{}{}$", patternConfigurations, patternPlatforms)] = defs[Defs::TargetCMakeToolset];

	defs[Defs::TargetChaletLocation] = R"json({
		"type": "string",
		"description": "The folder path of the root chalet.json for the project."
	})json"_ojson;

	defs[Defs::TargetChaletBuildFile] = R"json({
		"type": "string",
		"description": "The build file to use, if not chalet.json, relative to the location."
	})json"_ojson;

	defs[Defs::TargetChaletRecheck] = R"json({
		"type": "boolean",
		"description": "If true, Chalet will be invoked each time during the build."
	})json"_ojson;

	auto targetChalet = R"json({
		"type": "object",
		"additionalProperties": false,
		"required": [
			"type",
			"location"
		],
		"description": "Build the location with cmake"
	})json"_ojson;
	targetChalet[kProperties]["description"] = defs[Defs::TargetDescription];
	targetChalet[kProperties]["location"] = defs[Defs::TargetChaletLocation];
	targetChalet[kProperties]["buildFile"] = defs[Defs::TargetChaletBuildFile];
	targetChalet[kProperties]["recheck"] = defs[Defs::TargetChaletRecheck];
	targetChalet[kProperties]["type"] = defs[Defs::TargetType];
	targetChalet[kProperties]["onlyInConfiguration"] = defs[Defs::TargetOnlyInConfiguration];
	targetChalet[kProperties]["notInConfiguration"] = defs[Defs::TargetNotInConfiguration];
	targetChalet[kProperties]["onlyInPlatform"] = defs[Defs::TargetOnlyInPlatform];
	targetChalet[kProperties]["notInPlatform"] = defs[Defs::TargetNotInPlatform];
	targetChalet[kPatternProperties][fmt::format("^description{}{}$", patternConfigurations, patternPlatforms)] = defs[Defs::TargetDescription];
	targetChalet[kPatternProperties][fmt::format("^buildFile{}{}$", patternConfigurations, patternPlatforms)] = defs[Defs::TargetChaletBuildFile];

	//
	ret[kProperties] = Json::object();
	ret[kPatternProperties] = Json::object();

	ret[kPatternProperties]["^abstracts:[a-z]+$"] = targetProject;
	ret[kPatternProperties]["^abstracts:[a-z]+$"][kDescription] = "An abstract build project. 'abstracts:all' is a special project that gets implicitely added to each project";

	ret[kProperties]["abstracts"] = R"json({
		"type": "object",
		"additionalProperties": false,
		"description": "A list of abstract build projects"
	})json"_ojson;
	ret[kProperties]["abstracts"][kPatternProperties][R"(^[A-Za-z_-]+$)"] = targetProject;
	ret[kProperties]["abstracts"][kPatternProperties][R"(^[A-Za-z_-]+$)"][kDescription] = "An abstract build project. 'all' is implicitely added to each project.";

	ret[kProperties]["distribution"] = R"json({
		"type": "object",
		"additionalProperties": false,
		"description": "A list of bundle descriptors for the distribution."
	})json"_ojson;
	ret[kProperties]["distribution"][kPatternProperties][patternDistributionName] = R"json({
		"description": "A single bundle or script."
	})json"_ojson;
	ret[kProperties]["distribution"][kPatternProperties][patternDistributionName][kOneOf][0] = targetScript;
	ret[kProperties]["distribution"][kPatternProperties][patternDistributionName][kOneOf][1] = distributionBundle;

	auto configuration = R"json({
		"type": "object",
		"additionalProperties": false
	})json"_ojson;
	configuration[kProperties]["debugSymbols"] = defs[Defs::ConfigDebugSymbols];
	configuration[kProperties]["enableProfiling"] = defs[Defs::ConfigEnableProfiling];
	configuration[kProperties]["linkTimeOptimization"] = defs[Defs::ConfigLinkTimeOptimizations];
	configuration[kProperties]["optimizations"] = defs[Defs::ConfigOptimizationLevel];
	configuration[kProperties]["stripSymbols"] = defs[Defs::ConfigStripSymbols];

	ret[kProperties]["configurations"] = R"json({
		"anyOf": [
			{
				"type": "object",
				"additionalProperties": false,
				"description": "A list of allowed build configurations"
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
	ret[kProperties]["configurations"][kAnyOf][0][kPatternProperties][R"(^[A-Za-z]{3,}$)"] = configuration;

	ret[kProperties]["externalDependencies"] = R"json({
		"type": "object",
		"additionalProperties": false,
		"description": "A sequential list of externalDependencies to install prior to building or via the configure command. The key will be the destination directory name for the repository within the folder defined in 'externalDepDir'."
	})json"_ojson;
	ret[kProperties]["externalDependencies"][kPatternProperties]["^[\\w\\-\\+\\.]{3,100}$"] = externalDependency;

	const auto kTargets = "targets";
	ret[kProperties][kTargets] = R"json({
		"type": "object",
		"additionalProperties": false,
		"description": "A sequential list of projects, cmake projects, or scripts."
	})json"_ojson;
	ret[kProperties][kTargets][kPatternProperties][patternProjectName] = R"json({
		"description": "A single build target or script.",
		"oneOf": [
			{
				"type": "object"
			},
			{
				"type": "object"
			},
			{
				"type": "object",
				"properties": {},
				 "allOf": [
					{
						"if": {
							"properties": { "type": { "const": "CMake" } }
						},
						"then": {}
					},
					{
						"if": {
							"properties": { "type": { "const": "Chalet" } }
						},
						"then": {}
					}
				]
			}
		]
	})json"_ojson;
	ret[kProperties][kTargets][kPatternProperties][patternProjectName][kOneOf][0] = targetProject;
	ret[kProperties][kTargets][kPatternProperties][patternProjectName][kOneOf][1] = targetScript;
	ret[kProperties][kTargets][kPatternProperties][patternProjectName][kOneOf][2][kProperties]["type"] = defs[Defs::TargetType];
	ret[kProperties][kTargets][kPatternProperties][patternProjectName][kOneOf][2][kAllOf][0]["then"] = targetCMake;
	ret[kProperties][kTargets][kPatternProperties][patternProjectName][kOneOf][2][kAllOf][1]["then"] = targetChalet;

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

	ret[kProperties]["path"] = defs[Defs::EnvironmentPath];
	ret[kPatternProperties][fmt::format("^path{}{}$", patternConfigurations, patternPlatforms)] = defs[Defs::EnvironmentPath];

	return ret;
}
}
