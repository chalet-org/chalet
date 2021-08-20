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
/*****************************************************************************/
SchemaBuildJson::SchemaBuildJson() :
	kDefinitions("definitions"),
	kItems("items"),
	kProperties("properties"),
	kPattern("pattern"),
	kPatternProperties("patternProperties"),
	kDescription("description"),
	// kEnum("enum"),
	kExamples("examples"),
	kAnyOf("anyOf"),
	kAllOf("allOf"),
	kOneOf("oneOf"),
	kPatternProjectName(R"(^[\w\-\+\.]{3,}$)"),
	kPatternProjectLinks(R"(^[\w\-\+\.]+$)"),
	kPatternDistributionName(R"(^[\w\-\+\.\ \(\)]{3,}$)"),
	kPatternConfigurations(R"((:!?debug|))"),
	kPatternPlatforms(R"((\.!?windows|\.!?macos|\.!?linux|))")
{
}

/*****************************************************************************/
SchemaBuildJson::DefinitionMap SchemaBuildJson::getDefinitions()
{
	DefinitionMap defs;

	//
	// configurations
	//
	defs[Defs::ConfigurationDebugSymbols] = R"json({
		"type": "boolean",
		"description": "true to include debug symbols, false otherwise.\nIn GNU-based compilers, this is equivalent to the '-g3' option (-g & macro expansion information) and forces '-O0' if the optimizationLevel is not '0' or 'debug'.\nIn MSVC, this enables '/debug', '/incremental' and forces '/Od' if the optimizationLevel is not '0' or 'debug'.\nThis flag is also the determining factor whether the ':debug' suffix is used in a chalet.json property.",
		"default": false
	})json"_ojson;

	defs[Defs::ConfigurationEnableProfiling] = R"json({
		"type": "boolean",
		"description": "true to enable profiling for this configuration, false otherwise.\nIn GNU-based compilers, this is equivalent to the '-pg' option.\nIn MSVC, this doesn't do anything yet.\nIf profiling is enabled and the project is run, a compatible profiler application will be launched when the program is run.",
		"default": false
	})json"_ojson;

	defs[Defs::ConfigurationLinkTimeOptimizations] = R"json({
		"type": "boolean",
		"description": "true to use link-time optimization, false otherwise.\nIn GNU-based compilers, this is equivalent to passing the '-flto' option to the linker.\nIn MSVC, this enables the '/GL' option (Whole Program Optimization - which implies link-time optimization).",
		"default": false
	})json"_ojson;

	defs[Defs::ConfigurationOptimizationLevel] = R"json({
		"type": "string",
		"description": "The optimization level of the build.\nIn GNU-based compilers, This maps 1:1 with its respective '-O' option, except for 'debug' (-Od) and 'size' (-Os).\nIn MSVC, it's mapped as follows: 0 (/Od), 1 (/O1), 2 (/O2), 3 (/Ox), size (/Os), fast (/Ot), debug (/Od)\nIf this value is unset, no optimization level will be used (implying the compiler's default).",
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

	defs[Defs::ConfigurationStripSymbols] = R"json({
		"type": "boolean",
		"description": "true to strip symbols from the build, false otherwise.\nIn GNU-based compilers, this is equivalent to passing the '-s' option at link time. In MSVC, this is not applicable (symbols are stored in .pdb files).",
		"default": false
	})json"_ojson;

	//
	// distribution
	//
	defs[Defs::DistributionTargetConfiguration] = R"json({
		"type": "string",
		"description": "The name of the build configuration to use for this distribution target.\nIf this property is omitted, the 'Release' configuration will be used. In the case where custom configurations are defined, the first configuration without 'debugSymbols' and 'enableProfiling' is used.",
		"default": "Release"
	})json"_ojson;

	defs[Defs::DistributionTargetInclude] = R"json({
		"type": "array",
		"description": "A list of files or folders to copy into the output directory of the distribution target.\nIn MacOS, these will be placed into the 'Resources' folder of the application bundle.",
		"uniqueItems": true,
		"minItems": 1,
		"items": {
			"type": "string",
			"description": "A single file or folder to copy."
		}
	})json"_ojson;

	defs[Defs::DistributionTargetDescription] = R"json({
		"type": "string",
		"description": "TODO"
	})json"_ojson;

	defs[Defs::DistributionTargetExclude] = R"json({
		"type": "array",
		"description": "In folder paths that are included with 'include', exclude certain files or paths.\nCan accept a glob pattern.",
		"uniqueItems": true,
		"minItems": 1,
		"items": {
			"type": "string"
		}
	})json"_ojson;

	defs[Defs::DistributionTargetIncludeDependentSharedLibraries] = R"json({
		"type": "boolean",
		"description": "If true (default), any shared libraries that the bundle depeends on will also be copied into the bundle.",
		"default": true
	})json"_ojson;

	defs[Defs::DistributionTargetLinux] = R"json({
		"type": "object",
		"description": "Properties to describe the Linux distribution. At the moment, these only apply to desktop environments that support the XDG Desktop Entry Specification",
		"additionalProperties": false,
		"required": [
			"icon",
			"desktopEntry"
		],
		"properties": {
			"desktopEntry": {
				"type": "string",
				"description": "The location to an XDG Desktop Entry template. If the file does not exist, a basic one will be generated in its place."
			},
			"icon": {
				"type": "string",
				"description": "The location to an icon to use for the application (PNG 256x256 is recommended)"
			}
		}
	})json"_ojson;

	defs[Defs::DistributionTargetMacOS] = R"json({
		"type": "object",
		"description": "Properties to describe the MacOS distribution. Only one application bundle can be defined per distribution target.",
		"additionalProperties": false,
		"required": [
			"bundleName"
		],
		"properties": {
			"bundleName": {
				"type": "string",
				"description": "The short name of the macos bundle (required)"
			},
			"dmg": {
				"type": "object",
				"description": "If defined, a .dmg image will be created after the application bundle.",
				"additionalProperties": false,
				"properties": {
					"background": {
						"description": "If creating a .dmg image with 'makeDmg', this will define a background image for it.",
						"anyOf": [
							{
								"type": "string"
							},
							{
								"type": "object",
								"additionalProperties": false,
								"required": [
									"1x"
								],
								"properties": {
									"1x": {
										"type": "string",
										"description": "The path to a background image in PNG format created for 1x pixel density."
									},
									"2x": {
										"type": "string",
										"description": "The path to a background image in PNG format created for 2x pixel density."
									}
								}
							}
						]
					}
				}
			},
			"icon": {
				"type": "string",
				"description": "The path to an application icon either in PNG or ICNS format.\nIf the file is a .png, it will get converted to .icns during the bundle process."
			},
			"infoPropertyList": {
				"description": "The path to a .plist file, property list .json file, or an object of properties to export as a plist defining the distribution target.",
				"anyOf": [
					{
						"type": "string"
					},
					{
						"type": "object"
					}
				]
			}
		}
	})json"_ojson;
	defs[Defs::DistributionTargetMacOS]["properties"]["infoPropertyList"]["anyOf"][1]["default"] = JsonComments::parseLiteral(PlatformFileTemplates::macosInfoPlist());

	defs[Defs::DistributionTargetMainProject] = R"json({
		"type": "string",
		"description": "The name of the main executable project target.\nIf this property is not defined, the first executable in the 'projects' array of the distribution target will be chosen as the main executable."
	})json"_ojson;

	defs[Defs::DistributionTargetOutputDirectory] = R"json({
		"type": "string",
		"description": "The output folder to place the final build along with all of its included resources and shared libraries.",
		"default": "dist"
	})json"_ojson;

	defs[Defs::DistributionTargetProjects] = R"json({
		"type": "array",
		"uniqueItems": true,
		"description": "An array of project target names to include in this distribution target.\nIf 'mainProject' is not defined, the first executable target in this list will be chosen as the main exectuable.",
		"minItems": 1,
		"items": {
			"type": "string",
			"description": "The name of the project target."
		}
	})json"_ojson;
	defs[Defs::DistributionTargetProjects][kItems][kPattern] = kPatternProjectName;

	defs[Defs::DistributionTargetWindows] = R"json({
		"type": "object",
		"description": "Properties to describe the Windows distribution.\nAt the moment, metadata like versioning and descriptions are typically added during the build phase via an application manifest.",
		"additionalProperties": false,
		"properties": {
			"nsisScript": {
				"type": "string",
				"description": "Relative path to an NSIS installer script (.nsi) to compile for this distribution target, if the Nullsoft installer is available.\nThis is mainly for convenience, as one can also write their own batch script to do something like this and use that as a distribution target."
			}
		}
	})json"_ojson;

	//
	// externalDependency
	//
	defs[Defs::ExternalDependencyGitRepository] = R"json({
		"type": "string",
		"description": "The url of the git repository.",
		"pattern": "^(?:git|ssh|git\\+ssh|https?|git@[-\\w.]+):(\\/\\/)?(.*?)(\\.git)(\\/?|\\#[-\\d\\w._]+?)$"
	})json"_ojson;

	defs[Defs::ExternalDependencyGitBranch] = R"json({
		"type": "string",
		"description": "The branch to checkout. Uses the repository's default if not set."
	})json"_ojson;

	defs[Defs::ExternalDependencyGitCommit] = R"json({
		"type": "string",
		"description": "The SHA1 hash of the commit to checkout.",
		"pattern": "^[0-9a-f]{7,40}$"
	})json"_ojson;

	defs[Defs::ExternalDependencyGitTag] = R"json({
		"type": "string",
		"description": "The tag to checkout on the selected branch. If it's blank or not found, the head of the branch will be checked out."
	})json"_ojson;

	defs[Defs::ExternalDependencyGitSubmodules] = R"json({
		"type": "boolean",
		"description": "Do submodules need to be cloned?",
		"default": false
	})json"_ojson;

	//
	// other
	//
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

	//
	// target
	//
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
	defs[Defs::TargetNotInPlatform][kOneOf][0] = defs.at(Defs::EnumPlatform);
	defs[Defs::TargetNotInPlatform][kOneOf][1][kItems] = defs.at(Defs::EnumPlatform);

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
	defs[Defs::TargetOnlyInPlatform][kOneOf][0] = defs.at(Defs::EnumPlatform);
	defs[Defs::TargetOnlyInPlatform][kOneOf][1][kItems] = defs.at(Defs::EnumPlatform);

	defs[Defs::ProjectTargetExtends] = R"json({
		"type": "string",
		"description": "A project template to extend. Defaults to 'all' implicitly.",
		"pattern": "^[A-Za-z_-]+$",
		"default": "all"
	})json"_ojson;

	defs[Defs::ProjectTargetFiles] = R"json({
		"type": "array",
		"uniqueItems": true,
		"minItems": 1,
		"description": "Explicitly define the source files, relative to the working directory.",
		"items": {
			"type": "string"
		}
	})json"_ojson;

	defs[Defs::ProjectTargetKind] = R"json({
		"type": "string",
		"description": "The type of the project's compiled binary.",
		"enum": [
			"staticLibrary",
			"sharedLibrary",
			"consoleApplication",
			"desktopApplication"
		]
	})json"_ojson;

	defs[Defs::ProjectTargetLanguage] = R"json({
		"type": "string",
		"description": "The target language of the project.",
		"enum": [
			"C",
			"C++"
		],
		"default": "C++"
	})json"_ojson;

	defs[Defs::ProjectTargetRunProject] = R"json({
		"type": "boolean",
		"description": "Is this the main project to run during run-related commands (buildrun & run)?\n\nIf multiple targets are defined as true, the first will be chosen to run. If a command-line runProject is given, it will be prioritized.",
		"default": false
	})json"_ojson;

	defs[Defs::ProjectTargetRunArguments] = R"json({
		"type": "array",
		"description": "If the project is the run target, a string of arguments to pass to the run command.",
		"minItems": 1,
		"items": {
			"type": "string"
		}
	})json"_ojson;

	defs[Defs::ProjectTargetRunDependencies] = R"json({
		"type": "array",
		"uniqueItems": true,
		"description": "If the project is the run target, a list of dynamic libraries that should be copied before running.",
		"minItems": 1,
		"items": {
			"type": "string"
		}
	})json"_ojson;

	defs[Defs::ProjectTargetCxxCStandard] = R"json({
		"type": "string",
		"description": "The C standard to use in the compilation",
		"pattern": "^((c|gnu)\\d[\\dx]|(iso9899:(1990|199409|1999|199x|20\\d{2})))$",
		"default": "c11"
	})json"_ojson;

	defs[Defs::ProjectTargetCxxCompileOptions] = R"json({
		"type": "array",
		"uniqueItems": true,
		"minItems": 1,
		"description": "Options to add during the compilation step.",
		"items": {
			"type": "string"
		}
	})json"_ojson;

	defs[Defs::ProjectTargetCxxCppStandard] = R"json({
		"type": "string",
		"description": "The C++ standard to use in the compilation",
		"pattern": "^(c|gnu)\\+\\+\\d[\\dxyzab]$",
		"default": "c++17"
	})json"_ojson;

	defs[Defs::ProjectTargetCxxDefines] = R"json({
		"type": "array",
		"uniqueItems": true,
		"minItems": 1,
		"description": "Macro definitions to be used by the preprocessor",
		"items": {
			"type": "string"
		}
	})json"_ojson;

	defs[Defs::ProjectTargetCxxIncludeDirs] = R"json({
		"type": "array",
		"uniqueItems": true,
		"minItems": 1,
		"description": "A list of directories to include with the project.",
		"items": {
			"type": "string"
		}
	})json"_ojson;

	defs[Defs::ProjectTargetCxxLibDirs] = R"json({
		"type": "array",
		"uniqueItems": true,
		"minItems": 1,
		"description": "Fallback search paths to look for static or dynamic libraries (/usr/lib is included by default)",
		"items": {
			"type": "string"
		}
	})json"_ojson;

	defs[Defs::ProjectTargetCxxLinkerScript] = R"json({
		"type": "string",
		"description": "An LD linker script path (.ld file) to pass to the linker command"
	})json"_ojson;

	defs[Defs::ProjectTargetCxxLinkerOptions] = R"json({
		"type": "array",
		"uniqueItems": true,
		"minItems": 1,
		"description": "Options to add during the linking step.",
		"items": {
			"type": "string"
		}
	})json"_ojson;

	defs[Defs::ProjectTargetCxxLinks] = R"json({
		"type": "array",
		"uniqueItems": true,
		"minItems": 1,
		"description": "A list of dynamic links to use with the linker",
		"items": {
			"type": "string"
		}
	})json"_ojson;
	defs[Defs::ProjectTargetCxxLinks][kItems][kPattern] = kPatternProjectLinks;

	defs[Defs::ProjectTargetLocation] = R"json({
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
	defs[Defs::ProjectTargetLocation][kOneOf][2][kPatternProperties][fmt::format("^exclude{}{}$", kPatternConfigurations, kPatternPlatforms)] = R"json({
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
	defs[Defs::ProjectTargetLocation][kOneOf][2][kPatternProperties][fmt::format("^include{}{}$", kPatternConfigurations, kPatternPlatforms)] = R"json({
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

	defs[Defs::ProjectTargetCxxMacOsFrameworkPaths] = R"json({
		"type": "array",
		"description": "A list of paths to search for MacOS Frameworks",
		"uniqueItems": true,
		"minItems": 1,
		"items": {
			"type": "string"
		}
	})json"_ojson;

	defs[Defs::ProjectTargetCxxMacOsFrameworks] = R"json({
		"type": "array",
		"description": "A list of MacOS Frameworks to link to the project",
		"uniqueItems": true,
		"minItems": 1,
		"items": {
			"type": "string"
		}
	})json"_ojson;

	defs[Defs::ProjectTargetCxxObjectiveCxx] = R"json({
		"type": "boolean",
		"description": "Set to true if compiling Objective-C or Objective-C++ files (.m or .mm), or including any Objective-C/C++ headers.",
		"default": false
	})json"_ojson;

	defs[Defs::ProjectTargetCxxPrecompiledHeader] = R"json({
		"type": "string",
		"description": "Compile a header file as a pre-compiled header and include it in compilation of every object file in the project. Define a path relative to the workspace root."
	})json"_ojson;

	defs[Defs::ProjectTargetCxxThreads] = R"json({
		"type": "string",
		"enum": [
			"auto",
			"posix",
			"none"
		],
		"default": "auto"
	})json"_ojson;

	defs[Defs::ProjectTargetCxxRunTimeTypeInfo] = R"json({
		"type": "boolean",
		"description": "true to include run-time type information (default), false to exclude.",
		"default": true
	})json"_ojson;

	defs[Defs::ProjectTargetCxxExceptions] = R"json({
		"type": "boolean",
		"description": "true to use exceptions (default), false to turn off exceptions.",
		"default": true
	})json"_ojson;

	defs[Defs::ProjectTargetCxxStaticLinking] = R"json({
		"description": "true to statically link against compiler libraries (libc++, etc.). false to dynamically link them.",
		"type": "boolean",
		"default": false
	})json"_ojson;

	defs[Defs::ProjectTargetCxxStaticLinks] = R"json({
		"type": "array",
		"description": "A list of static links to use with the linker",
		"uniqueItems": true,
		"minItems": 1,
		"items": {
			"type": "string"
		}
	})json"_ojson;
	defs[Defs::ProjectTargetCxxStaticLinks][kItems][kPattern] = kPatternProjectLinks;

	defs[Defs::ProjectTargetCxxWarnings] = R"json({
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

	defs[Defs::ProjectTargetCxxWarnings][kAnyOf][1][kItems][kExamples] = {
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

	defs[Defs::ProjectTargetCxxWindowsAppManifest] = R"json({
		"description": "The path to a Windows application manifest. Only applies to application (kind=[consoleApplication|desktopApplication]) and shared library (kind=sharedLibrary) targets",
		"type": "string"
	})json"_ojson;

	defs[Defs::ProjectTargetCxxWindowsAppIcon] = R"json({
		"type": "string",
		"description": "The windows icon to use for the project. Only applies to application targets (kind=[consoleApplication|desktopApplication])"
	})json"_ojson;

	defs[Defs::ProjectTargetCxxWindowsOutputDef] = R"json({
		"type": "boolean",
		"description": "If true for a shared library (kind=sharedLibrary) target on Windows, a .def file will be created",
		"default": false
	})json"_ojson;

	defs[Defs::ProjectTargetCxxWindowsPrefixOutputFilename] = R"json({
		"type": "boolean",
		"description": "Only applies to shared library targets (kind=sharedLibrary) on windows. If true, prefixes the output dll with 'lib'. This may not be desirable with standalone dlls.",
		"default": true
	})json"_ojson;

	defs[Defs::ScriptTargetScript] = R"json({
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

	defs[Defs::TargetType] = R"json({
		"type": "string",
		"description": "The target type, if not a local project or script.",
		"enum": ["CMake", "Chalet"]
	})json"_ojson;

	defs[Defs::CMakeTargetLocation] = R"json({
		"type": "string",
		"description": "The folder path of the root CMakeLists.txt for the project."
	})json"_ojson;

	defs[Defs::CMakeTargetBuildFile] = R"json({
		"type": "string",
		"description": "The build file to use, if not CMakeLists.txt, relative to the location. (-C)"
	})json"_ojson;

	defs[Defs::CMakeTargetDefines] = R"json({
		"type": "array",
		"description": "Macro definitions to be passed into CMake. (-D)",
		"uniqueItems": true,
		"minItems": 1,
		"items": {
			"type": "string"
		}
	})json"_ojson;

	defs[Defs::CMakeTargetRecheck] = R"json({
		"type": "boolean",
		"description": "If true, CMake will be invoked each time during the build.",
		"default": false
	})json"_ojson;

	defs[Defs::CMakeTargetToolset] = R"json({
		"type": "string",
		"description": "A toolset to be passed to CMake with the -T option."
	})json"_ojson;

	defs[Defs::ChaletTargetLocation] = R"json({
		"type": "string",
		"description": "The folder path of the root chalet.json for the project."
	})json"_ojson;

	defs[Defs::ChaletTargetBuildFile] = R"json({
		"type": "string",
		"description": "The build file to use, if not chalet.json, relative to the location."
	})json"_ojson;

	defs[Defs::ChaletTargetRecheck] = R"json({
		"type": "boolean",
		"description": "If true, Chalet will be invoked each time during the build."
	})json"_ojson;

	//
	// Complex Definitions
	//
	{

		auto configuration = R"json({
			"type": "object",
			"additionalProperties": false,
			"description": "Properties to describe a single build configuration type."
		})json"_ojson;
		configuration[kProperties]["debugSymbols"] = getDefinition(Defs::ConfigurationDebugSymbols);
		configuration[kProperties]["enableProfiling"] = getDefinition(Defs::ConfigurationEnableProfiling);
		configuration[kProperties]["linkTimeOptimization"] = getDefinition(Defs::ConfigurationLinkTimeOptimizations);
		configuration[kProperties]["optimizationLevel"] = getDefinition(Defs::ConfigurationOptimizationLevel);
		configuration[kProperties]["stripSymbols"] = getDefinition(Defs::ConfigurationStripSymbols);
		defs[Defs::Configuration] = std::move(configuration);
	}

	{
		auto distDef = R"json({
			"type": "object",
			"additionalProperties": false,
			"description": "Properties to describe an individual distribution target."
		})json"_ojson;
		distDef[kProperties] = Json::object();
		distDef[kProperties]["configuration"] = getDefinition(Defs::DistributionTargetConfiguration);
		distDef[kProperties]["description"] = getDefinition(Defs::DistributionTargetDescription);
		distDef[kProperties]["exclude"] = getDefinition(Defs::DistributionTargetExclude);
		distDef[kProperties]["include"] = getDefinition(Defs::DistributionTargetInclude);
		distDef[kProperties]["includeDependentSharedLibraries"] = getDefinition(Defs::DistributionTargetIncludeDependentSharedLibraries);
		distDef[kProperties]["linux"] = getDefinition(Defs::DistributionTargetLinux);
		distDef[kProperties]["macos"] = getDefinition(Defs::DistributionTargetMacOS);
		distDef[kProperties]["windows"] = getDefinition(Defs::DistributionTargetWindows);
		distDef[kProperties]["mainProject"] = getDefinition(Defs::DistributionTargetMainProject);
		distDef[kProperties]["outDir"] = getDefinition(Defs::DistributionTargetOutputDirectory);
		distDef[kProperties]["projects"] = getDefinition(Defs::DistributionTargetProjects);
		distDef[kPatternProperties][fmt::format("^include{}{}$", kPatternConfigurations, kPatternPlatforms)] = getDefinition(Defs::DistributionTargetInclude);
		distDef[kPatternProperties][fmt::format("^exclude{}{}$", kPatternConfigurations, kPatternPlatforms)] = getDefinition(Defs::DistributionTargetExclude);
		defs[Defs::DistributionTarget] = std::move(distDef);
	}

	{
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
		externalDependency[kOneOf][0][kProperties]["repository"] = getDefinition(Defs::ExternalDependencyGitRepository);
		externalDependency[kOneOf][0][kProperties]["submodules"] = getDefinition(Defs::ExternalDependencyGitSubmodules);
		externalDependency[kOneOf][0][kProperties]["tag"] = getDefinition(Defs::ExternalDependencyGitTag);

		externalDependency[kOneOf][1][kProperties] = Json::object();
		externalDependency[kOneOf][1][kProperties]["repository"] = getDefinition(Defs::ExternalDependencyGitRepository);
		externalDependency[kOneOf][1][kProperties]["submodules"] = getDefinition(Defs::ExternalDependencyGitSubmodules);
		externalDependency[kOneOf][1][kProperties]["branch"] = getDefinition(Defs::ExternalDependencyGitBranch);
		externalDependency[kOneOf][1][kProperties]["commit"] = getDefinition(Defs::ExternalDependencyGitCommit);
		defs[Defs::ExternalDependency] = std::move(externalDependency);
	}

	{
		auto targetProjectCxx = R"json({
			"type": "object",
			"additionalProperties": false
		})json"_ojson;
		targetProjectCxx[kProperties]["cStandard"] = getDefinition(Defs::ProjectTargetCxxCStandard);
		targetProjectCxx[kProperties]["compileOptions"] = getDefinition(Defs::ProjectTargetCxxCompileOptions);
		targetProjectCxx[kProperties]["cppStandard"] = getDefinition(Defs::ProjectTargetCxxCppStandard);
		targetProjectCxx[kProperties]["defines"] = getDefinition(Defs::ProjectTargetCxxDefines);
		targetProjectCxx[kProperties]["includeDirs"] = getDefinition(Defs::ProjectTargetCxxIncludeDirs);
		targetProjectCxx[kProperties]["libDirs"] = getDefinition(Defs::ProjectTargetCxxLibDirs);
		targetProjectCxx[kProperties]["linkerScript"] = getDefinition(Defs::ProjectTargetCxxLinkerScript);
		targetProjectCxx[kProperties]["linkerOptions"] = getDefinition(Defs::ProjectTargetCxxLinkerOptions);
		targetProjectCxx[kProperties]["links"] = getDefinition(Defs::ProjectTargetCxxLinks);
		targetProjectCxx[kProperties]["macosFrameworkPaths"] = getDefinition(Defs::ProjectTargetCxxMacOsFrameworkPaths);
		targetProjectCxx[kProperties]["macosFrameworks"] = getDefinition(Defs::ProjectTargetCxxMacOsFrameworks);
		targetProjectCxx[kProperties]["objectiveCxx"] = getDefinition(Defs::ProjectTargetCxxObjectiveCxx);
		targetProjectCxx[kProperties]["pch"] = getDefinition(Defs::ProjectTargetCxxPrecompiledHeader);
		targetProjectCxx[kProperties]["threads"] = getDefinition(Defs::ProjectTargetCxxThreads);
		targetProjectCxx[kProperties]["rtti"] = getDefinition(Defs::ProjectTargetCxxRunTimeTypeInfo);
		targetProjectCxx[kProperties]["exceptions"] = getDefinition(Defs::ProjectTargetCxxExceptions);
		targetProjectCxx[kProperties]["staticLinking"] = getDefinition(Defs::ProjectTargetCxxStaticLinking);
		targetProjectCxx[kProperties]["staticLinks"] = getDefinition(Defs::ProjectTargetCxxStaticLinks);
		targetProjectCxx[kProperties]["warnings"] = getDefinition(Defs::ProjectTargetCxxWarnings);
		targetProjectCxx[kProperties]["windowsPrefixOutputFilename"] = getDefinition(Defs::ProjectTargetCxxWindowsPrefixOutputFilename);
		targetProjectCxx[kProperties]["windowsOutputDef"] = getDefinition(Defs::ProjectTargetCxxWindowsOutputDef);
		targetProjectCxx[kProperties]["windowsApplicationIcon"] = getDefinition(Defs::ProjectTargetCxxWindowsAppIcon);
		targetProjectCxx[kProperties]["windowsApplicationManifest"] = getDefinition(Defs::ProjectTargetCxxWindowsAppManifest);

		targetProjectCxx[kPatternProperties][fmt::format("^cStandard{}$", kPatternPlatforms)] = getDefinition(Defs::ProjectTargetCxxCStandard);
		targetProjectCxx[kPatternProperties][fmt::format("^cppStandard{}$", kPatternPlatforms)] = getDefinition(Defs::ProjectTargetCxxCppStandard);
		targetProjectCxx[kPatternProperties][fmt::format("^compileOptions{}$", kPatternPlatforms)] = getDefinition(Defs::ProjectTargetCxxCompileOptions);
		targetProjectCxx[kPatternProperties][fmt::format("^defines{}{}$", kPatternConfigurations, kPatternPlatforms)] = getDefinition(Defs::ProjectTargetCxxDefines);
		targetProjectCxx[kPatternProperties][fmt::format("^includeDirs{}$", kPatternPlatforms)] = getDefinition(Defs::ProjectTargetCxxIncludeDirs);
		targetProjectCxx[kPatternProperties][fmt::format("^libDirs{}$", kPatternPlatforms)] = getDefinition(Defs::ProjectTargetCxxLibDirs);
		targetProjectCxx[kPatternProperties][fmt::format("^linkerScript{}$", kPatternPlatforms)] = getDefinition(Defs::ProjectTargetCxxLinkerScript);
		targetProjectCxx[kPatternProperties][fmt::format("^linkerOptions{}$", kPatternPlatforms)] = getDefinition(Defs::ProjectTargetCxxLinkerOptions);
		targetProjectCxx[kPatternProperties][fmt::format("^links{}{}$", kPatternConfigurations, kPatternPlatforms)] = getDefinition(Defs::ProjectTargetCxxLinks);
		targetProjectCxx[kPatternProperties][fmt::format("^objectiveCxx{}$", kPatternPlatforms)] = getDefinition(Defs::ProjectTargetCxxObjectiveCxx);
		targetProjectCxx[kPatternProperties][fmt::format("^staticLinks{}{}$", kPatternConfigurations, kPatternPlatforms)] = getDefinition(Defs::ProjectTargetCxxStaticLinks);
		targetProjectCxx[kPatternProperties][fmt::format("^threads{}{}$", kPatternConfigurations, kPatternPlatforms)] = getDefinition(Defs::ProjectTargetCxxThreads);
		targetProjectCxx[kPatternProperties][fmt::format("^rtti{}{}$", kPatternConfigurations, kPatternPlatforms)] = getDefinition(Defs::ProjectTargetCxxRunTimeTypeInfo);
		targetProjectCxx[kPatternProperties][fmt::format("^exceptions{}{}$", kPatternConfigurations, kPatternPlatforms)] = getDefinition(Defs::ProjectTargetCxxExceptions);
		targetProjectCxx[kPatternProperties][fmt::format("^staticLinking{}{}$", kPatternConfigurations, kPatternPlatforms)] = getDefinition(Defs::ProjectTargetCxxStaticLinking);
		defs[Defs::ProjectTargetCxx] = std::move(targetProjectCxx);
	}

	{
		auto targetProject = R"json({
			"type": "object",
			"additionalProperties": false
		})json"_ojson;
		targetProject[kProperties]["settings:Cxx"] = getDefinition(Defs::ProjectTargetCxx);
		targetProject[kProperties]["extends"] = getDefinition(Defs::ProjectTargetExtends);
		targetProject[kProperties]["files"] = getDefinition(Defs::ProjectTargetFiles);
		targetProject[kProperties]["kind"] = getDefinition(Defs::ProjectTargetKind);
		targetProject[kProperties]["language"] = getDefinition(Defs::ProjectTargetLanguage);
		targetProject[kProperties]["location"] = getDefinition(Defs::ProjectTargetLocation);
		targetProject[kProperties]["onlyInConfiguration"] = getDefinition(Defs::TargetOnlyInConfiguration);
		targetProject[kProperties]["notInConfiguration"] = getDefinition(Defs::TargetNotInConfiguration);
		targetProject[kProperties]["onlyInPlatform"] = getDefinition(Defs::TargetOnlyInPlatform);
		targetProject[kProperties]["notInPlatform"] = getDefinition(Defs::TargetNotInPlatform);
		targetProject[kProperties]["runProject"] = getDefinition(Defs::ProjectTargetRunProject);
		targetProject[kProperties]["runArguments"] = getDefinition(Defs::ProjectTargetRunArguments);
		targetProject[kProperties]["runDependencies"] = getDefinition(Defs::ProjectTargetRunDependencies);
		targetProject[kPatternProperties][fmt::format("^runProject{}{}$", kPatternConfigurations, kPatternPlatforms)] = getDefinition(Defs::ProjectTargetRunProject);
		targetProject[kPatternProperties][fmt::format("^runDependencies{}{}$", kPatternConfigurations, kPatternPlatforms)] = getDefinition(Defs::ProjectTargetRunDependencies);
		defs[Defs::ProjectTarget] = std::move(targetProject);
	}

	{
		auto targetScript = R"json({
			"type": "object",
			"additionalProperties": false
		})json"_ojson;
		targetScript[kProperties]["script"] = getDefinition(Defs::ScriptTargetScript);
		targetScript[kProperties]["script"][kDescription] = "Script(s) to run during this build step.";
		targetScript[kProperties]["description"] = getDefinition(Defs::TargetDescription);
		{
			auto scriptPattern = fmt::format("^script{}{}$", kPatternConfigurations, kPatternPlatforms);
			targetScript[kPatternProperties][scriptPattern] = getDefinition(Defs::ScriptTargetScript);
			targetScript[kPatternProperties][scriptPattern][kDescription] = "Script(s) to run during this build step.";
		}
		targetScript[kPatternProperties][fmt::format("^description{}{}$", kPatternConfigurations, kPatternPlatforms)] = getDefinition(Defs::TargetDescription);
		defs[Defs::ScriptTarget] = std::move(targetScript);
	}

	{

		auto targetCMake = R"json({
			"type": "object",
			"additionalProperties": false,
			"required": [
				"type",
				"location"
			],
			"description": "Build the location with CMake"
		})json"_ojson;
		targetCMake[kProperties]["description"] = getDefinition(Defs::TargetDescription);
		targetCMake[kProperties]["location"] = getDefinition(Defs::CMakeTargetLocation);
		targetCMake[kProperties]["buildFile"] = getDefinition(Defs::CMakeTargetBuildFile);
		targetCMake[kProperties]["defines"] = getDefinition(Defs::CMakeTargetDefines);
		targetCMake[kProperties]["toolset"] = getDefinition(Defs::CMakeTargetToolset);
		targetCMake[kProperties]["recheck"] = getDefinition(Defs::CMakeTargetRecheck);
		targetCMake[kProperties]["type"] = getDefinition(Defs::TargetType);
		targetCMake[kProperties]["onlyInConfiguration"] = getDefinition(Defs::TargetOnlyInConfiguration);
		targetCMake[kProperties]["notInConfiguration"] = getDefinition(Defs::TargetNotInConfiguration);
		targetCMake[kProperties]["onlyInPlatform"] = getDefinition(Defs::TargetOnlyInPlatform);
		targetCMake[kProperties]["notInPlatform"] = getDefinition(Defs::TargetNotInPlatform);
		targetCMake[kPatternProperties][fmt::format("^description{}{}$", kPatternConfigurations, kPatternPlatforms)] = getDefinition(Defs::TargetDescription);
		targetCMake[kPatternProperties][fmt::format("^buildFile{}{}$", kPatternConfigurations, kPatternPlatforms)] = getDefinition(Defs::CMakeTargetBuildFile);
		targetCMake[kPatternProperties][fmt::format("^defines{}{}$", kPatternConfigurations, kPatternPlatforms)] = getDefinition(Defs::CMakeTargetDefines);
		targetCMake[kPatternProperties][fmt::format("^toolset{}{}$", kPatternConfigurations, kPatternPlatforms)] = getDefinition(Defs::CMakeTargetToolset);
		defs[Defs::CMakeTarget] = std::move(targetCMake);
	}

	{

		auto targetChalet = R"json({
			"type": "object",
			"additionalProperties": false,
			"required": [
				"type",
				"location"
			],
			"description": "Build the location with Chalet"
		})json"_ojson;
		targetChalet[kProperties]["description"] = getDefinition(Defs::TargetDescription);
		targetChalet[kProperties]["location"] = getDefinition(Defs::ChaletTargetLocation);
		targetChalet[kProperties]["buildFile"] = getDefinition(Defs::ChaletTargetBuildFile);
		targetChalet[kProperties]["recheck"] = getDefinition(Defs::ChaletTargetRecheck);
		targetChalet[kProperties]["type"] = getDefinition(Defs::TargetType);
		targetChalet[kProperties]["onlyInConfiguration"] = getDefinition(Defs::TargetOnlyInConfiguration);
		targetChalet[kProperties]["notInConfiguration"] = getDefinition(Defs::TargetNotInConfiguration);
		targetChalet[kProperties]["onlyInPlatform"] = getDefinition(Defs::TargetOnlyInPlatform);
		targetChalet[kProperties]["notInPlatform"] = getDefinition(Defs::TargetNotInPlatform);
		targetChalet[kPatternProperties][fmt::format("^description{}{}$", kPatternConfigurations, kPatternPlatforms)] = getDefinition(Defs::TargetDescription);
		targetChalet[kPatternProperties][fmt::format("^buildFile{}{}$", kPatternConfigurations, kPatternPlatforms)] = getDefinition(Defs::ChaletTargetBuildFile);
		defs[Defs::ChaletTarget] = std::move(targetChalet);
	}

	return defs;
}

/*****************************************************************************/
std::string SchemaBuildJson::getDefinitionName(const Defs inDef)
{
	switch (inDef)
	{
		case Defs::Configuration: return "configuration";
		case Defs::ConfigurationDebugSymbols: return "config-debugSymbols";
		case Defs::ConfigurationEnableProfiling: return "config-enableProfiling";
		case Defs::ConfigurationLinkTimeOptimizations: return "config-linkTimeOptimizations";
		case Defs::ConfigurationOptimizationLevel: return "config-optimizationLevel";
		case Defs::ConfigurationStripSymbols: return "config-stripSymbols";
		//
		case Defs::DistributionTarget: return "distribution-target";
		case Defs::DistributionTargetConfiguration: return "distribution-target-configuration";
		case Defs::DistributionTargetInclude: return "distribution-target-include";
		case Defs::DistributionTargetDescription: return "distribution-target-description";
		case Defs::DistributionTargetExclude: return "distribution-target-exclude";
		case Defs::DistributionTargetIncludeDependentSharedLibraries: return "distribution-target-includeDependentSharedLibraries";
		case Defs::DistributionTargetLinux: return "distribution-target-linux";
		case Defs::DistributionTargetMacOS: return "distribution-target-macos";
		case Defs::DistributionTargetMainProject: return "distribution-target-mainProject";
		case Defs::DistributionTargetOutputDirectory: return "distribution-target-outDir";
		case Defs::DistributionTargetProjects: return "distribution-target-projects";
		case Defs::DistributionTargetWindows: return "distribution-target-windows";
		//
		case Defs::ExternalDependency: return "external-dependency";
		case Defs::ExternalDependencyGitRepository: return "external-git-repository";
		case Defs::ExternalDependencyGitBranch: return "external-git-branch";
		case Defs::ExternalDependencyGitCommit: return "external-git-commit";
		case Defs::ExternalDependencyGitTag: return "external-git-tag";
		case Defs::ExternalDependencyGitSubmodules: return "external-git-submodules";
		//
		case Defs::EnumPlatform: return "enum-platform";
		//
		case Defs::EnvironmentPath: return "environment-path";
		//
		case Defs::TargetDescription: return "target-description";
		case Defs::TargetType: return "target-type";
		case Defs::TargetNotInConfiguration: return "target-notInConfiguration";
		case Defs::TargetNotInPlatform: return "target-notInPlatform";
		case Defs::TargetOnlyInConfiguration: return "target-onlyInConfiguration";
		case Defs::TargetOnlyInPlatform: return "target-onlyInPlatform";
		//
		case Defs::ProjectTargetExtends: return "target-project-extends";
		case Defs::ProjectTargetFiles: return "target-project-files";
		case Defs::ProjectTargetKind: return "target-project-kind";
		case Defs::ProjectTargetLocation: return "target-project-location";
		case Defs::ProjectTargetLanguage: return "target-project-language";
		case Defs::ProjectTargetRunProject: return "target-project-runProject";
		case Defs::ProjectTargetRunArguments: return "target-project-runArguments";
		case Defs::ProjectTargetRunDependencies: return "target-project-runDependencies";
		//
		case Defs::ProjectTarget: return "target-project";
		case Defs::ProjectTargetCxx: return "target-project-cxx";
		case Defs::ProjectTargetCxxCStandard: return "target-project-cxx-cStandard";
		case Defs::ProjectTargetCxxCppStandard: return "target-project-cxx-cppStandard";
		case Defs::ProjectTargetCxxCompileOptions: return "target-project-cxx-compileOptions";
		case Defs::ProjectTargetCxxDefines: return "target-project-cxx-defines";
		case Defs::ProjectTargetCxxIncludeDirs: return "target-project-cxx-includeDirs";
		case Defs::ProjectTargetCxxLibDirs: return "target-project-cxx-libDirs";
		case Defs::ProjectTargetCxxLinkerScript: return "target-project-cxx-linkerScript";
		case Defs::ProjectTargetCxxLinkerOptions: return "target-project-cxx-linkerOptions";
		case Defs::ProjectTargetCxxLinks: return "target-project-cxx-links";
		case Defs::ProjectTargetCxxMacOsFrameworkPaths: return "target-project-cxx-macosFrameworkPaths";
		case Defs::ProjectTargetCxxMacOsFrameworks: return "target-project-cxx-macosFrameworks";
		case Defs::ProjectTargetCxxObjectiveCxx: return "target-project-cxx-objectiveCxx";
		case Defs::ProjectTargetCxxPrecompiledHeader: return "target-project-cxx-pch";
		case Defs::ProjectTargetCxxThreads: return "target-project-cxx-threads";
		case Defs::ProjectTargetCxxRunTimeTypeInfo: return "target-project-cxx-rtti";
		case Defs::ProjectTargetCxxExceptions: return "target-project-cxx-exceptions";
		case Defs::ProjectTargetCxxStaticLinking: return "target-project-cxx-staticLinking";
		case Defs::ProjectTargetCxxStaticLinks: return "target-project-cxx-staticLinks";
		case Defs::ProjectTargetCxxWarnings: return "target-project-cxx-warnings";
		case Defs::ProjectTargetCxxWindowsAppManifest: return "target-project-cxx-windowsAppManifest";
		case Defs::ProjectTargetCxxWindowsAppIcon: return "target-project-cxx-windowsAppIcon";
		case Defs::ProjectTargetCxxWindowsOutputDef: return "target-project-cxx-windowsOutputDef";
		case Defs::ProjectTargetCxxWindowsPrefixOutputFilename: return "target-project-cxx-windowsPrefixOutputFilename";
		//
		case Defs::ScriptTarget: return "target-script";
		case Defs::ScriptTargetScript: return "target-script-script";
		//
		case Defs::CMakeTarget: return "target-cmake";
		case Defs::CMakeTargetLocation: return "target-cmake-location";
		case Defs::CMakeTargetBuildFile: return "target-cmake-buildFile";
		case Defs::CMakeTargetDefines: return "target-cmake-defines";
		case Defs::CMakeTargetRecheck: return "target-cmake-recheck";
		case Defs::CMakeTargetToolset: return "target-cmake-toolset";
		//
		case Defs::ChaletTarget: return "target-chalet";
		case Defs::ChaletTargetLocation: return "target-chalet-location";
		case Defs::ChaletTargetBuildFile: return "target-chalet-buildFile";
		case Defs::ChaletTargetRecheck: return "target-chalet-recheck";

		default: break;
	}

	chalet_assert(false, "");

	return std::string();
}

/*****************************************************************************/
Json SchemaBuildJson::getDefinition(const Defs inDef)
{
	if (m_useRefs)
	{
		const auto name = getDefinitionName(inDef);
		Json ret = Json::object();
		ret["$ref"] = fmt::format("#/definitions/{}", name);

		return ret;
	}
	else
	{
		return m_defs.at(inDef);
	}
}

/*****************************************************************************/
Json SchemaBuildJson::get()
{
	Json ret;
	ret["$schema"] = "http://json-schema.org/draft-07/schema";
	ret["type"] = "object";
	ret["additionalProperties"] = false;
	ret["required"] = {
		"version",
		"workspace",
		"targets"
	};

	if (m_defs.empty())
		m_defs = getDefinitions();

	if (m_useRefs)
	{
		ret[kDefinitions] = Json::object();
		for (auto& [def, defJson] : m_defs)
		{
			const auto name = getDefinitionName(def);
			ret[kDefinitions][name] = defJson;
		}
	}

	//
	ret[kProperties] = Json::object();
	ret[kPatternProperties] = Json::object();

	ret[kPatternProperties]["^abstracts:[a-z]+$"] = getDefinition(Defs::ProjectTarget);
	ret[kPatternProperties]["^abstracts:[a-z]+$"][kDescription] = "An abstract build project. 'abstracts:all' is a special project that gets implicitely added to each project";

	ret[kProperties]["abstracts"] = R"json({
		"type": "object",
		"additionalProperties": false,
		"description": "A list of abstract build projects"
	})json"_ojson;
	ret[kProperties]["abstracts"][kPatternProperties][R"(^[A-Za-z_-]+$)"] = getDefinition(Defs::ProjectTarget);
	ret[kProperties]["abstracts"][kPatternProperties][R"(^[A-Za-z_-]+$)"][kDescription] = "An abstract build project. 'all' is implicitely added to each project.";

	ret[kProperties]["configurations"] = R"json({
		"description": "An array of allowed build configuration presets, or an object of custom build configurations.",
		"default": [
			"Release",
			"Debug",
			"RelWithDebInfo",
			"MinSizeRel",
			"Profile"
		],
		"anyOf": [
			{
				"type": "object",
				"additionalProperties": false
			},
			{
				"type": "array",
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
	ret[kProperties]["configurations"][kAnyOf][0][kPatternProperties][R"(^[A-Za-z]{3,}$)"] = getDefinition(Defs::Configuration);

	ret[kProperties]["distribution"] = R"json({
		"type": "object",
		"additionalProperties": false,
		"description": "A sequential list of distribution targets to be created during the bundle phase."
	})json"_ojson;
	ret[kProperties]["distribution"][kPatternProperties][kPatternDistributionName] = R"json({
		"description": "A single distribution target or script."
	})json"_ojson;
	ret[kProperties]["distribution"][kPatternProperties][kPatternDistributionName][kOneOf][0] = getDefinition(Defs::ScriptTarget);
	ret[kProperties]["distribution"][kPatternProperties][kPatternDistributionName][kOneOf][1] = getDefinition(Defs::DistributionTarget);

	ret[kProperties]["externalDepDir"] = R"json({
		"type": "string",
		"description": "The path to install external dependencies into (see externalDependencies).",
		"default": "chalet_external"
	})json"_ojson;

	ret[kProperties]["externalDependencies"] = R"json({
		"type": "object",
		"additionalProperties": false,
		"description": "A sequential list of externalDependencies to install prior to building or via the configure command. The key will be the destination directory name for the repository within the folder defined in 'externalDepDir'."
	})json"_ojson;
	ret[kProperties]["externalDependencies"][kPatternProperties]["^[\\w\\-\\+\\.]{3,100}$"] = getDefinition(Defs::ExternalDependency);

	ret[kProperties]["path"] = getDefinition(Defs::EnvironmentPath);
	ret[kPatternProperties][fmt::format("^path{}{}$", kPatternConfigurations, kPatternPlatforms)] = getDefinition(Defs::EnvironmentPath);

	const auto targets = "targets";
	ret[kProperties][targets] = R"json({
		"type": "object",
		"additionalProperties": false,
		"description": "A sequential list of projects, cmake projects, or scripts."
	})json"_ojson;
	ret[kProperties][targets][kPatternProperties][kPatternProjectName] = R"json({
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
	ret[kProperties][targets][kPatternProperties][kPatternProjectName][kOneOf][0] = getDefinition(Defs::ProjectTarget);
	ret[kProperties][targets][kPatternProperties][kPatternProjectName][kOneOf][1] = getDefinition(Defs::ScriptTarget);
	ret[kProperties][targets][kPatternProperties][kPatternProjectName][kOneOf][2][kProperties]["type"] = getDefinition(Defs::TargetType);
	ret[kProperties][targets][kPatternProperties][kPatternProjectName][kOneOf][2][kAllOf][0]["then"] = getDefinition(Defs::CMakeTarget);
	ret[kProperties][targets][kPatternProperties][kPatternProjectName][kOneOf][2][kAllOf][1]["then"] = getDefinition(Defs::ChaletTarget);

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

	return ret;
}
}
