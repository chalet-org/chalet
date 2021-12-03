/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "BuildJson/SchemaBuildJson.hpp"

#include "FileTemplates/PlatformFileTemplates.hpp"

#include "State/BuildConfiguration.hpp"
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
	kAdditionalProperties("additionalProperties"),
	kPattern("pattern"),
	kPatternProperties("patternProperties"),
	kDescription("description"),
	kDefault("default"),
	kEnum("enum"),
	kExamples("examples"),
	// kAnyOf("anyOf"),
	// kAllOf("allOf"),
	kOneOf("oneOf"),
	kThen("then"),
	kElse("else"),
	kPatternTargetName(R"(^[\w\-+.]{3,}$)"),
	kPatternAbstractName(R"([A-Za-z-_]+)"),
	kPatternSourceTargetLinks(R"(^[\w\-+.]+$)"),
	kPatternDistributionName(R"(^[\w\-+. ()]{3,}$)"),
	kPatternConditionConfigurations(R"regex((\.!?(debug)\b\.?)?)regex"),
	kPatternConditionPlatforms(R"regex((\.!?(windows|macos|linux)\b){1,2})regex"),
	kPatternConditionConfigurationsPlatforms(R"regex((\.!?(debug|windows|macos|linux)\b){1,2})regex"),
	kPatternConditionPlatformsInner(R"regex((!?(windows|macos|linux)\b))regex"),
	kPatternConditionConfigurationsPlatformsInner(R"regex((!?(debug|windows|macos|linux)\b){1,2})regex"),
	kPatternCompilers(R"regex(^(\*|[\w\-+.]{3,})(\.!?(debug|windows|macos|linux)\b){0,2}$)regex")
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
		"description": "true to use link-time optimization, false otherwise.\nIn GNU-based compilers, this is equivalent to passing the '-flto' option to the linker.",
		"default": false
	})json"_ojson;

	defs[Defs::ConfigurationOptimizationLevel] = R"json({
		"type": "string",
		"description": "The optimization level of the build.\nIn GNU-based compilers, This maps 1:1 with its respective '-O' option, except for 'debug' (-Od) and 'size' (-Os).\nIn MSVC, it's mapped as follows: 0 (/Od), 1 (/O1), 2 (/O2), 3 (/Ox), size (/Os), fast (/Ot), debug (/Od)\nIf this value is unset, no optimization level will be used (implying the compiler's default).",
		"minLength": 1,
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
	defs[Defs::DistributionTargetKind] = R"json({
		"type": "string",
		"description": "Whether the distribution target is a bundle or script.",
		"minLength": 1,
		"enum": [
			"bundle",
			"script",
			"archive"
		]
	})json"_ojson;

	defs[Defs::DistributionTargetConfiguration] = R"json({
		"type": "string",
		"description": "The name of the build configuration to use for this distribution target.\nIf this property is omitted, the 'Release' configuration will be used. In the case where custom configurations are defined, the first configuration without 'debugSymbols' and 'enableProfiling' is used.",
		"minLength": 1,
		"default": "Release"
	})json"_ojson;

	defs[Defs::DistributionTargetInclude] = R"json({
		"type": "array",
		"description": "A list of files or folders to copy into the output directory of the distribution target.\nIn MacOS, these will be placed into the 'Resources' folder of the application bundle.",
		"uniqueItems": true,
		"minItems": 1,
		"items": {
			"type": "string",
			"description": "A single file or folder to copy.",
			"minLength": 1
		}
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
		"description": "If true (default), any shared libraries that the bundle depeends on will also be copied.",
		"default": true
	})json"_ojson;

	defs[Defs::DistributionTargetLinux] = R"json({
		"type": "object",
		"description": "Properties to describe the Linux distribution. At the moment, these only apply to desktop environments that support the XDG Desktop Entry Specification",
		"additionalProperties": false,
		"properties": {
			"desktopEntry": {
				"type": "string",
				"description": "The location to an XDG Desktop Entry template. If the file does not exist, a basic one will be generated in its place.",
				"minLength": 1
			},
			"icon": {
				"type": "string",
				"description": "The location to an icon to use for the application (PNG 256x256 is recommended)",
				"minLength": 1
			}
		}
	})json"_ojson;

	defs[Defs::DistributionTargetMacOS] = R"json({
		"type": "object",
		"description": "Properties to describe the MacOS distribution. Only one application bundle can be defined per distribution target.",
		"additionalProperties": false,
		"required": [
			"bundleType"
		],
		"properties": {
			"bundleType": {
				"type": "string",
				"description": "The MacOS bundle type (only .app is supported currently)",
				"minLength": 1,
				"enum": [
					"app"
				],
				"default": "app"
			},
			"dmg": {
				"type": "object",
				"description": "If defined, a .dmg image will be created after the application bundle.",
				"additionalProperties": false,
				"properties": {
					"background": {
						"description": "If creating a .dmg image with 'makeDmg', this will define a background image for it.",
						"oneOf": [
							{
								"type": "string",
								"minLength": 1
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
										"description": "The path to a background image in PNG format created for 1x pixel density.",
										"minLength": 1
									},
									"2x": {
										"type": "string",
										"description": "The path to a background image in PNG format created for 2x pixel density.",
										"minLength": 1
									}
								}
							}
						]
					}
				}
			},
			"icon": {
				"type": "string",
				"description": "The path to an application icon either in PNG or ICNS format.\nIf the file is a .png, it will get converted to .icns during the bundle process.",
				"minLength": 1
			},
			"infoPropertyList": {
				"description": "The path to a .plist file, property list .json file, or an object of properties to export as a plist defining the distribution target.",
				"oneOf": [
					{
						"type": "string",
						"minLength": 1
					},
					{
						"type": "object"
					}
				]
			}
		}
	})json"_ojson;
	defs[Defs::DistributionTargetMacOS]["properties"]["infoPropertyList"][kOneOf][1]["default"] = JsonComments::parseLiteral(PlatformFileTemplates::macosInfoPlist());

	defs[Defs::DistributionTargetMainExecutable] = R"json({
		"type": "string",
		"description": "The name of the main executable project target.\nIf this property is not defined, the first executable in the 'targets' array of the distribution target will be chosen as the main executable.",
		"minLength": 1
	})json"_ojson;

	defs[Defs::DistributionTargetOutputDirectory] = R"json({
		"type": "string",
		"description": "The output folder to place the final build along with all of its included resources and shared libraries.",
		"minLength": 1,
		"default": "dist"
	})json"_ojson;

	defs[Defs::DistributionTargetBuildTargets] = R"json({
		"type": "array",
		"uniqueItems": true,
		"description": "An array of build target names to include in this distribution target.\nIf 'mainExecutable' is not defined, the first executable target in this list will be chosen as the main exectuable.",
		"minItems": 1,
		"items": {
			"type": "string",
			"description": "The name of the build target.",
			"minLength": 1
		}
	})json"_ojson;
	defs[Defs::DistributionTargetBuildTargets][kItems][kPattern] = kPatternTargetName;

	defs[Defs::DistributionTargetWindows] = R"json({
		"type": "object",
		"description": "Properties to describe the Windows distribution.\nAt the moment, metadata like versioning and descriptions are typically added during the build phase via an application manifest.",
		"additionalProperties": false,
		"properties": {
			"nsisScript": {
				"type": "string",
				"description": "Relative path to an NSIS installer script (.nsi) to compile for this distribution target, if the Nullsoft installer is available.\nThis is mainly for convenience, as one can also write their own batch script to do something like this and use that as a distribution target.",
				"minLength": 1
			}
		}
	})json"_ojson;

	//
	defs[Defs::DistArchiveTargetBundles] = R"json({
		"type": "array",
		"description": "distribution bundle targets to include in the zip archive",
		"uniqueItems": true,
		"minItems": 1,
		"items": {
			"type": "string",
			"minLength": 1
		}
	})json"_ojson;

	//
	// externalDependency
	//
	defs[Defs::ExternalDependencyGitRepository] = R"json({
		"type": "string",
		"description": "The url of the git repository.",
		"minLength": 1
	})json"_ojson;
	defs[Defs::ExternalDependencyGitRepository][kPattern] = R"regex(^(?:git|ssh|git\+ssh|https?|git@[\w\-.]+):(\/\/)?(.*?)(\.git)(\/?|#[\w\d\-._]+?)$)regex";

	defs[Defs::ExternalDependencyGitBranch] = R"json({
		"type": "string",
		"description": "The branch to checkout. Uses the repository's default if not set.",
		"minLength": 1
	})json"_ojson;

	defs[Defs::ExternalDependencyGitCommit] = R"json({
		"type": "string",
		"description": "The SHA1 hash of the commit to checkout.",
		"pattern": "^[0-9a-f]{7,40}$",
		"minLength": 1
	})json"_ojson;

	defs[Defs::ExternalDependencyGitTag] = R"json({
		"type": "string",
		"description": "The tag to checkout on the selected branch. If it's blank or not found, the head of the branch will be checked out.",
		"minLength": 1
	})json"_ojson;

	defs[Defs::ExternalDependencyGitSubmodules] = R"json({
		"type": "boolean",
		"description": "Do submodules need to be cloned?",
		"default": false
	})json"_ojson;

	//
	// other
	//
	defs[Defs::EnvironmentSearchPaths] = R"json({
		"type": "array",
		"description": "Any additional search paths to include. Accepts Chalet variables such as ${buildDir} & ${externalDir}",
		"uniqueItems": true,
		"minItems": 1,
		"items": {
			"type": "string",
			"minLength": 1
		}
	})json"_ojson;

	//
	// target
	//
	defs[Defs::TargetDescription] = R"json({
		"type": "string",
		"description": "A description of the target to display during the build.",
		"minLength": 1
	})json"_ojson;

	defs[Defs::TargetCondition] = R"json({
		"type": "string",
		"description": "A rule describing when to include this target in the build.",
		"minLength": 1
	})json"_ojson;
	defs[Defs::TargetCondition][kPattern] = fmt::format("^{}$", kPatternConditionConfigurationsPlatformsInner);

	defs[Defs::SourceTargetExtends] = R"json({
		"type": "string",
		"description": "A project template to extend. Defaults to '*' implicitly.",
		"pattern": "",
		"minLength": 1,
		"default": "*"
	})json"_ojson;
	defs[Defs::SourceTargetExtends][kPattern] = fmt::format("^{}$", kPatternAbstractName);

	defs[Defs::SourceTargetFiles] = R"json({
		"type": "array",
		"uniqueItems": true,
		"minItems": 1,
		"description": "Explicitly define the source files, relative to the working directory.",
		"items": {
			"type": "string",
			"minLength": 1
		}
	})json"_ojson;

	defs[Defs::TargetKind] = R"json({
		"type": "string",
		"description": "The type of the target's compiled binary, a script or external project.",
		"minLength": 1,
		"enum": [
			"staticLibrary",
			"sharedLibrary",
			"executable",
			"cmakeProject",
			"chaletProject",
			"script"
		]
	})json"_ojson;

	defs[Defs::SourceTargetLanguage] = R"json({
		"type": "string",
		"description": "The target language of the project.",
		"minLength": 1,
		"enum": [
			"C",
			"C++",
			"Objective-C",
			"Objective-C++"
		],
		"default": "C++"
	})json"_ojson;

	defs[Defs::TargetRunTarget] = R"json({
		"type": "boolean",
		"description": "Is this the main project to run during run-related commands (buildrun & run)?\n\nIf multiple targets are defined as true, the first will be chosen to run. If a command-line runTarget is given, it will be prioritized. If no executable targets are defined as the runTarget, the first executable one will be chosen.",
		"default": false
	})json"_ojson;

	defs[Defs::TargetRunTargetArguments] = R"json({
		"type": "array",
		"description": "If the project is the run target, a string of arguments to pass to the run command.",
		"minItems": 1,
		"items": {
			"type": "string",
			"minLength": 1
		}
	})json"_ojson;

	defs[Defs::TargetRunDependencies] = R"json({
		"type": "array",
		"uniqueItems": true,
		"description": "If the project is the run target, a list of dynamic libraries that should be copied before running.",
		"minItems": 1,
		"items": {
			"type": "string",
			"minLength": 1
		}
	})json"_ojson;

	defs[Defs::SourceTargetCxxCStandard] = R"json({
		"type": "string",
		"description": "The C standard to use in the compilation",
		"pattern": "^((c|gnu)\\d[\\dx]|(iso9899:(1990|199409|1999|199x|20\\d{2})))$",
		"minLength": 1,
		"default": "c11"
	})json"_ojson;

	defs[Defs::SourceTargetCxxCompileOptions] = R"json({
		"type": "object",
		"additionalProperties": false,
		"description": "Addtional options (per compiler type) to add during the compilation step."
	})json"_ojson;
	defs[Defs::SourceTargetCxxCompileOptions][kPatternProperties][kPatternCompilers] = R"json({
		"type": "string",
		"minLength": 1
	})json"_ojson;

	defs[Defs::SourceTargetCxxCppStandard] = R"json({
		"type": "string",
		"description": "The C++ standard to use in the compilation",
		"pattern": "^(c|gnu)\\+\\+\\d[\\dxyzab]$",
		"minLength": 1,
		"default": "c++17"
	})json"_ojson;

	defs[Defs::SourceTargetCxxDefines] = R"json({
		"type": "array",
		"uniqueItems": true,
		"minItems": 1,
		"description": "Macro definitions to be used by the preprocessor",
		"items": {
			"type": "string",
			"minLength": 1
		}
	})json"_ojson;

	defs[Defs::SourceTargetCxxIncludeDirs] = R"json({
		"type": "array",
		"uniqueItems": true,
		"minItems": 1,
		"description": "A list of directories to include with the project.",
		"items": {
			"type": "string",
			"minLength": 1
		}
	})json"_ojson;

	defs[Defs::SourceTargetCxxLibDirs] = R"json({
		"type": "array",
		"uniqueItems": true,
		"minItems": 1,
		"description": "Fallback search paths to look for static or dynamic libraries (/usr/lib is included by default)",
		"items": {
			"type": "string",
			"minLength": 1
		}
	})json"_ojson;

	defs[Defs::SourceTargetCxxLinkerScript] = R"json({
		"type": "string",
		"description": "An LD linker script path (.ld file) to pass to the linker command",
		"minLength": 1
	})json"_ojson;

	defs[Defs::SourceTargetCxxLinkerOptions] = R"json({
		"type": "object",
		"additionalProperties": false,
		"description": "Addtional options (per compiler type) to add during the linking step."
	})json"_ojson;
	defs[Defs::SourceTargetCxxLinkerOptions][kPatternProperties][kPatternCompilers] = R"json({
		"type": "string",
		"minLength": 1
	})json"_ojson;

	defs[Defs::SourceTargetCxxLinks] = R"json({
		"type": "array",
		"uniqueItems": true,
		"minItems": 1,
		"description": "A list of dynamic links to use with the linker",
		"items": {
			"type": "string",
			"minLength": 1
		}
	})json"_ojson;
	defs[Defs::SourceTargetCxxLinks][kItems][kPattern] = kPatternSourceTargetLinks;

	defs[Defs::SourceTargetLocation] = R"json({
		"description": "The root path of the source files, relative to the working directory.",
		"oneOf": [
			{
				"type": "string",
				"minLength": 1
			},
			{
				"type": "array",
				"uniqueItems": true,
				"minItems": 1,
				"items": {
					"type": "string",
					"minLength": 1
				}
			},
			{
				"type": "object",
				"additionalProperties": false,
				"required": [
					"include"
				],
				"properties": {
					"include" : {
						"oneOf": [
							{
								"type": "string",
								"minLength": 1
							},
							{
								"type": "array",
								"uniqueItems": true,
								"minItems": 1,
								"items": {
									"type": "string",
									"minLength": 1
								}
							}
						]
					},
					"exclude" : {
						"oneOf": [
							{
								"type": "string",
								"minLength": 1
							},
							{
								"type": "array",
								"uniqueItems": true,
								"minItems": 1,
								"items": {
									"type": "string",
									"minLength": 1
								}
							}
						]
					}
				}
			}
		]
	})json"_ojson;
	defs[Defs::SourceTargetLocation][kOneOf][2][kPatternProperties][fmt::format("^exclude{}$", kPatternConditionConfigurationsPlatforms)] = defs[Defs::SourceTargetLocation][kOneOf][2][kProperties]["exclude"];
	defs[Defs::SourceTargetLocation][kOneOf][2][kPatternProperties][fmt::format("^include{}$", kPatternConditionConfigurationsPlatforms)] = defs[Defs::SourceTargetLocation][kOneOf][2][kProperties]["include"];

	defs[Defs::SourceTargetCxxMacOsFrameworkPaths] = R"json({
		"type": "array",
		"description": "A list of paths to search for MacOS Frameworks",
		"uniqueItems": true,
		"minItems": 1,
		"items": {
			"type": "string",
			"minLength": 1
		}
	})json"_ojson;

	defs[Defs::SourceTargetCxxMacOsFrameworks] = R"json({
		"type": "array",
		"description": "A list of MacOS Frameworks to link to the project",
		"uniqueItems": true,
		"minItems": 1,
		"items": {
			"type": "string",
			"minLength": 1
		}
	})json"_ojson;

	defs[Defs::SourceTargetCxxPrecompiledHeader] = R"json({
		"type": "string",
		"description": "Compile a header file as a pre-compiled header and include it in compilation of every object file in the project. Define a path relative to the workspace root.",
		"minLength": 1
	})json"_ojson;

	defs[Defs::SourceTargetCxxThreads] = R"json({
		"type": "string",
		"description": "The thread model to use.",
		"minLength": 1,
		"enum": [
			"auto",
			"posix",
			"none"
		],
		"default": "auto"
	})json"_ojson;

	defs[Defs::SourceTargetCxxCppModules] = R"json({
		"type": "boolean",
		"description": "true to enable C++20 modules, false to disable (default).",
		"default": false
	})json"_ojson;

	defs[Defs::SourceTargetCxxCppCoroutines] = R"json({
		"type": "boolean",
		"description": "true to enable C++20 coroutines, false to disable (default).",
		"default": false
	})json"_ojson;

	defs[Defs::SourceTargetCxxCppConcepts] = R"json({
		"type": "boolean",
		"description": "true to enable C++20 concepts in previous language standards (equivalent to '-fconcepts' or '-fconcepts-ts'), false to disable (default).",
		"default": false
	})json"_ojson;

	defs[Defs::SourceTargetCxxRunTimeTypeInfo] = R"json({
		"type": "boolean",
		"description": "true to include run-time type information (default), false to exclude.",
		"default": true
	})json"_ojson;

	defs[Defs::SourceTargetCxxExceptions] = R"json({
		"type": "boolean",
		"description": "true to use exceptions (default), false to turn off exceptions.",
		"default": true
	})json"_ojson;

	defs[Defs::SourceTargetCxxStaticLinking] = R"json({
		"description": "true to statically link against compiler libraries (libc++, etc.). false to dynamically link them.",
		"type": "boolean",
		"default": false
	})json"_ojson;

	defs[Defs::SourceTargetCxxStaticLinks] = R"json({
		"type": "array",
		"description": "A list of static links to use with the linker",
		"uniqueItems": true,
		"minItems": 1,
		"items": {
			"type": "string",
			"minLength": 1
		}
	})json"_ojson;
	defs[Defs::SourceTargetCxxStaticLinks][kItems][kPattern] = kPatternSourceTargetLinks;

	defs[Defs::SourceTargetCxxWarnings] = R"json({
		"description": "Either a preset of the warnings to use, or the warnings flags themselves (excluding '-W' prefix)",
		"oneOf": [
			{
				"type": "string",
				"minLength": 1,
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
			{},
			{
				"type": "array",
				"uniqueItems": true,
				"minItems": 1,
				"items": {
					"type": "string",
					"minLength": 1
				}
			}
		]
	})json"_ojson;
	defs[Defs::SourceTargetCxxWarnings][kOneOf][1] = R"json({
		"type": "object",
		"additionalProperties": false,
		"description": "Warnings specific to each compiler"
	})json"_ojson;
	defs[Defs::SourceTargetCxxWarnings][kOneOf][1][kPatternProperties][kPatternCompilers] = defs[Defs::SourceTargetCxxWarnings][kOneOf][2];

	defs[Defs::SourceTargetCxxWarnings][kOneOf][2][kItems][kExamples] = {
		"abi",
		"absolute-value",
		"address",
		"aggregate-return",
		"all",
		"alloc-size-larger-than=VAL",
		"alloc-zero",
		"alloca",
		"alloca-larger-than=VAL",
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
		"frame-larger-than=VAL",
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
		"larger-than=VAL",
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
		"stack-usage=VAL",
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
		"vla-larger-than=VAL",
		"vla-parameter",
		"volatile-register-var",
		"write-strings",
		"zero-as-null-pointer-constant",
		"zero-length-bounds"
	};
	defs[Defs::SourceTargetCxxWarnings][kOneOf][1][kPatternProperties][kPatternCompilers][kItems][kExamples] = defs[Defs::SourceTargetCxxWarnings][kOneOf][2][kItems][kExamples];

	defs[Defs::SourceTargetCxxWindowsAppManifest] = R"json({
		"description": "The path to a Windows application manifest, or false to disable automatic generation. Only applies to executable (kind=executable) and shared library (kind=sharedLibrary) targets",
		"oneOf": [
			{
				"type": "string",
				"minLength": 1
			},
			{
				"type": "boolean",
				"const": false
			}
		]
	})json"_ojson;

	defs[Defs::SourceTargetCxxWindowsAppIcon] = R"json({
		"type": "string",
		"description": "The windows icon to use for the project. Only applies to executable targets (kind=executable)",
		"minLength": 1
	})json"_ojson;

	/*defs[Defs::SourceTargetCxxWindowsOutputDef] = R"json({
		"type": "boolean",
		"description": "If true for a shared library (kind=sharedLibrary) target on Windows, a .def file will be created",
		"default": false
	})json"_ojson;*/

	defs[Defs::SourceTargetCxxWindowsSubSystem] = R"json({
		"type": "string",
		"description": "The subsystem to use for the target on Windows systems. If not specified, defaults to 'console'",
		"minLength": 1,
		"enum": [
			"console",
			"windows",
			"bootApplication",
			"native",
			"posix",
			"efiApplication",
			"efiBootServer",
			"efiRom",
			"efiRuntimeDriver"
		],
		"default": "console"
	})json"_ojson;

	defs[Defs::SourceTargetCxxWindowsEntryPoint] = R"json({
		"type": "string",
		"description": "The type of entry point to use for the target on Windows systems. If not specified, defaults to 'main'",
		"minLength": 1,
		"enum": [
			"main",
			"wmain",
			"WinMain",
			"wWinMain",
			"DllMain"
		],
		"default": "main"
	})json"_ojson;

	defs[Defs::ScriptTargetScript] = R"json({
		"description": "Script(s) to run during this build step.",
		"oneOf": [
			{
				"type": "string",
				"minLength": 1
			},
			{
				"type": "array",
				"uniqueItems": true,
				"minItems": 1,
				"items": {
					"type": "string",
					"minLength": 1
				}
			}
		]
	})json"_ojson;

	defs[Defs::CMakeTargetLocation] = R"json({
		"type": "string",
		"description": "The folder path of the root CMakeLists.txt for the project.",
		"minLength": 1
	})json"_ojson;

	defs[Defs::CMakeTargetBuildFile] = R"json({
		"type": "string",
		"description": "The build file to use, if not CMakeLists.txt, relative to the location. (-C)",
		"minLength": 1
	})json"_ojson;

	defs[Defs::CMakeTargetDefines] = R"json({
		"type": "array",
		"description": "Macro definitions to be passed into CMake. (-D)",
		"uniqueItems": true,
		"minItems": 1,
		"items": {
			"type": "string",
			"minLength": 1
		}
	})json"_ojson;

	defs[Defs::CMakeTargetRecheck] = R"json({
		"type": "boolean",
		"description": "If true, CMake will be invoked each time during the build.",
		"default": false
	})json"_ojson;

	defs[Defs::CMakeTargetToolset] = R"json({
		"type": "string",
		"description": "A toolset to be passed to CMake with the -T option.",
		"minLength": 1
	})json"_ojson;

	defs[Defs::ChaletTargetLocation] = R"json({
		"type": "string",
		"description": "The folder path of the root chalet.json for the project.",
		"minLength": 1
	})json"_ojson;

	defs[Defs::ChaletTargetBuildFile] = R"json({
		"type": "string",
		"description": "The build file to use, if not chalet.json, relative to the location.",
		"minLength": 1
	})json"_ojson;

	defs[Defs::ChaletTargetRecheck] = R"json({
		"type": "boolean",
		"description": "If true, Chalet will be invoked each time during the build."
	})json"_ojson;

	defs[Defs::CMakeTargetRunExecutable] = R"json({
		"type": "string",
		"description": "The path to an executable to run, relative to the build directory.",
		"minLength": 1
	})json"_ojson;

	auto getDefinitionwithCompilerOptions = [this](const Defs inDef) {
		Json ret = R"json({
			"oneOf": [
				{},
				{
					"type": "object",
					"additionalProperties": false,
					"description": "Options specific to each compiler"
				}
			]
		})json"_ojson;
		ret[kOneOf][0] = getDefinition(inDef);
		ret[kOneOf][1][kPatternProperties][kPatternCompilers] = ret[kOneOf][0];

		return ret;
	};

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
			"description": "Properties to describe an individual distribution target.",
			"anyOf": [
				{
					"required": [
						"kind",
						"buildTargets"
					]
				},
				{
					"required": [
						"kind",
						"include"
					]
				}
			]
		})json"_ojson;
		distDef[kProperties] = Json::object();
		distDef[kProperties]["kind"] = getDefinition(Defs::DistributionTargetKind);
		distDef[kProperties]["buildTargets"] = getDefinition(Defs::DistributionTargetBuildTargets);
		distDef[kProperties]["configuration"] = getDefinition(Defs::DistributionTargetConfiguration);
		distDef[kProperties]["description"] = getDefinition(Defs::TargetDescription);
		distDef[kProperties]["exclude"] = getDefinition(Defs::DistributionTargetExclude);
		distDef[kProperties]["include"] = getDefinition(Defs::DistributionTargetInclude);
		distDef[kProperties]["includeDependentSharedLibraries"] = getDefinition(Defs::DistributionTargetIncludeDependentSharedLibraries);
		distDef[kProperties]["linux"] = getDefinition(Defs::DistributionTargetLinux);
		distDef[kProperties]["macos"] = getDefinition(Defs::DistributionTargetMacOS);
		distDef[kProperties]["windows"] = getDefinition(Defs::DistributionTargetWindows);
		distDef[kProperties]["mainExecutable"] = getDefinition(Defs::DistributionTargetMainExecutable);
		distDef[kProperties]["subdirectory"] = getDefinition(Defs::DistributionTargetOutputDirectory);
		distDef[kPatternProperties][fmt::format("^description{}$", kPatternConditionConfigurationsPlatforms)] = getDefinition(Defs::TargetDescription);
		distDef[kPatternProperties][fmt::format("^include{}$", kPatternConditionConfigurationsPlatforms)] = getDefinition(Defs::DistributionTargetInclude);
		distDef[kPatternProperties][fmt::format("^exclude{}$", kPatternConditionConfigurationsPlatforms)] = getDefinition(Defs::DistributionTargetExclude);
		defs[Defs::DistributionTarget] = std::move(distDef);
	}

	{
		auto distArchiveDef = R"json({
			"type": "object",
			"additionalProperties": false,
			"description": "Properties to describe an individual distribution archive.",
			"required": [
				"kind",
				"bundles"
			]
		})json"_ojson;
		distArchiveDef[kProperties]["kind"] = getDefinition(Defs::DistributionTargetKind);
		distArchiveDef[kProperties]["bundles"] = getDefinition(Defs::DistArchiveTargetBundles);
		defs[Defs::DistArchiveTarget] = std::move(distArchiveDef);
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
		auto sourceTargetCxx = R"json({
			"type": "object",
			"additionalProperties": false
		})json"_ojson;
		sourceTargetCxx[kProperties]["cStandard"] = getDefinition(Defs::SourceTargetCxxCStandard);
		sourceTargetCxx[kProperties]["compileOptions"] = getDefinition(Defs::SourceTargetCxxCompileOptions);
		sourceTargetCxx[kProperties]["cppStandard"] = getDefinition(Defs::SourceTargetCxxCppStandard);
		sourceTargetCxx[kProperties]["defines"] = getDefinitionwithCompilerOptions(Defs::SourceTargetCxxDefines);
		sourceTargetCxx[kProperties]["includeDirs"] = getDefinitionwithCompilerOptions(Defs::SourceTargetCxxIncludeDirs);
		sourceTargetCxx[kProperties]["libDirs"] = getDefinitionwithCompilerOptions(Defs::SourceTargetCxxLibDirs);
		sourceTargetCxx[kProperties]["linkerScript"] = getDefinition(Defs::SourceTargetCxxLinkerScript);
		sourceTargetCxx[kProperties]["linkerOptions"] = getDefinition(Defs::SourceTargetCxxLinkerOptions);
		sourceTargetCxx[kProperties]["links"] = getDefinitionwithCompilerOptions(Defs::SourceTargetCxxLinks);
		sourceTargetCxx[kProperties]["macosFrameworkPaths"] = getDefinition(Defs::SourceTargetCxxMacOsFrameworkPaths);
		sourceTargetCxx[kProperties]["macosFrameworks"] = getDefinition(Defs::SourceTargetCxxMacOsFrameworks);
		sourceTargetCxx[kProperties]["pch"] = getDefinition(Defs::SourceTargetCxxPrecompiledHeader);
		sourceTargetCxx[kProperties]["threads"] = getDefinition(Defs::SourceTargetCxxThreads);
		sourceTargetCxx[kProperties]["cppModules"] = getDefinition(Defs::SourceTargetCxxCppModules);
		sourceTargetCxx[kProperties]["cppCoroutines"] = getDefinition(Defs::SourceTargetCxxCppCoroutines);
		sourceTargetCxx[kProperties]["cppConcepts"] = getDefinition(Defs::SourceTargetCxxCppConcepts);
		sourceTargetCxx[kProperties]["rtti"] = getDefinition(Defs::SourceTargetCxxRunTimeTypeInfo);
		sourceTargetCxx[kProperties]["exceptions"] = getDefinition(Defs::SourceTargetCxxExceptions);
		sourceTargetCxx[kProperties]["staticLinking"] = getDefinition(Defs::SourceTargetCxxStaticLinking);
		sourceTargetCxx[kProperties]["staticLinks"] = getDefinitionwithCompilerOptions(Defs::SourceTargetCxxStaticLinks);
		sourceTargetCxx[kProperties]["warnings"] = getDefinition(Defs::SourceTargetCxxWarnings);
		// sourceTargetCxx[kProperties]["windowsOutputDef"] = getDefinition(Defs::SourceTargetCxxWindowsOutputDef);
		sourceTargetCxx[kProperties]["windowsApplicationIcon"] = getDefinition(Defs::SourceTargetCxxWindowsAppIcon);
		sourceTargetCxx[kProperties]["windowsApplicationManifest"] = getDefinition(Defs::SourceTargetCxxWindowsAppManifest);
		sourceTargetCxx[kProperties]["windowsSubSystem"] = getDefinition(Defs::SourceTargetCxxWindowsSubSystem);
		sourceTargetCxx[kProperties]["windowsEntryPoint"] = getDefinition(Defs::SourceTargetCxxWindowsEntryPoint);

		sourceTargetCxx[kPatternProperties][fmt::format("^cStandard{}$", kPatternConditionPlatforms)] = getDefinition(Defs::SourceTargetCxxCStandard);
		sourceTargetCxx[kPatternProperties][fmt::format("^cppStandard{}$", kPatternConditionPlatforms)] = getDefinition(Defs::SourceTargetCxxCppStandard);
		sourceTargetCxx[kPatternProperties][fmt::format("^defines{}$", kPatternConditionConfigurationsPlatforms)] = getDefinition(Defs::SourceTargetCxxDefines);
		sourceTargetCxx[kPatternProperties][fmt::format("^includeDirs{}$", kPatternConditionPlatforms)] = getDefinition(Defs::SourceTargetCxxIncludeDirs);
		sourceTargetCxx[kPatternProperties][fmt::format("^libDirs{}$", kPatternConditionPlatforms)] = getDefinition(Defs::SourceTargetCxxLibDirs);
		sourceTargetCxx[kPatternProperties][fmt::format("^linkerScript{}$", kPatternConditionPlatforms)] = getDefinition(Defs::SourceTargetCxxLinkerScript);
		sourceTargetCxx[kPatternProperties][fmt::format("^links{}$", kPatternConditionConfigurationsPlatforms)] = getDefinition(Defs::SourceTargetCxxLinks);
		sourceTargetCxx[kPatternProperties][fmt::format("^staticLinks{}$", kPatternConditionConfigurationsPlatforms)] = getDefinition(Defs::SourceTargetCxxStaticLinks);
		sourceTargetCxx[kPatternProperties][fmt::format("^threads{}$", kPatternConditionConfigurationsPlatforms)] = getDefinition(Defs::SourceTargetCxxThreads);
		sourceTargetCxx[kPatternProperties][fmt::format("^cppModules{}$", kPatternConditionPlatforms)] = getDefinition(Defs::SourceTargetCxxCppModules);
		sourceTargetCxx[kPatternProperties][fmt::format("^cppCoroutines{}$", kPatternConditionPlatforms)] = getDefinition(Defs::SourceTargetCxxCppCoroutines);
		sourceTargetCxx[kPatternProperties][fmt::format("^cppConcepts{}$", kPatternConditionPlatforms)] = getDefinition(Defs::SourceTargetCxxCppConcepts);
		sourceTargetCxx[kPatternProperties][fmt::format("^rtti{}$", kPatternConditionConfigurationsPlatforms)] = getDefinition(Defs::SourceTargetCxxRunTimeTypeInfo);
		sourceTargetCxx[kPatternProperties][fmt::format("^exceptions{}$", kPatternConditionConfigurationsPlatforms)] = getDefinition(Defs::SourceTargetCxxExceptions);
		sourceTargetCxx[kPatternProperties][fmt::format("^staticLinking{}$", kPatternConditionConfigurationsPlatforms)] = getDefinition(Defs::SourceTargetCxxStaticLinking);

		sourceTargetCxx[kPatternProperties][fmt::format("^windowsApplicationIcon{}$", kPatternConditionConfigurations)] = getDefinition(Defs::SourceTargetCxxWindowsAppIcon);
		sourceTargetCxx[kPatternProperties][fmt::format("^windowsApplicationManifest{}$", kPatternConditionConfigurations)] = getDefinition(Defs::SourceTargetCxxWindowsAppManifest);
		sourceTargetCxx[kPatternProperties][fmt::format("^windowsSubSystem{}$", kPatternConditionConfigurations)] = getDefinition(Defs::SourceTargetCxxWindowsSubSystem);
		sourceTargetCxx[kPatternProperties][fmt::format("^windowsEntryPoint{}$", kPatternConditionConfigurations)] = getDefinition(Defs::SourceTargetCxxWindowsEntryPoint);

		defs[Defs::SourceTargetCxx] = std::move(sourceTargetCxx);
	}

	{
		auto abstractSource = R"json({
			"type": "object",
			"additionalProperties": false
		})json"_ojson;
		abstractSource[kProperties]["settings:Cxx"] = getDefinition(Defs::SourceTargetCxx);
		abstractSource[kProperties]["settings"] = R"json({
			"type": "object",
			"description": "Settings for each language",
			"additionalProperties": false
		})json"_ojson;
		abstractSource[kProperties]["settings"][kProperties]["Cxx"] = getDefinition(Defs::SourceTargetCxx);
		abstractSource[kProperties]["language"] = getDefinition(Defs::SourceTargetLanguage);
		abstractSource[kPatternProperties][fmt::format("^language{}$", kPatternConditionPlatforms)] = getDefinition(Defs::TargetDescription);
		defs[Defs::AbstractTarget] = std::move(abstractSource);
	}
	{
		auto targetSource = R"json({
			"type": "object",
			"additionalProperties": false,
			"required": [ "kind" ]
		})json"_ojson;
		targetSource[kProperties]["condition"] = getDefinition(Defs::TargetCondition);
		targetSource[kProperties]["description"] = getDefinition(Defs::TargetDescription);
		targetSource[kProperties]["extends"] = getDefinition(Defs::SourceTargetExtends);
		targetSource[kProperties]["files"] = getDefinition(Defs::SourceTargetFiles);
		targetSource[kProperties]["kind"] = getDefinition(Defs::TargetKind);
		targetSource[kProperties]["language"] = getDefinition(Defs::SourceTargetLanguage);
		targetSource[kProperties]["location"] = getDefinition(Defs::SourceTargetLocation);
		targetSource[kProperties]["settings"] = R"json({
			"type": "object",
			"description": "Settings for each language",
			"additionalProperties": false
		})json"_ojson;
		targetSource[kProperties]["settings"][kProperties]["Cxx"] = getDefinition(Defs::SourceTargetCxx);
		targetSource[kProperties]["settings:Cxx"] = getDefinition(Defs::SourceTargetCxx);
		targetSource[kPatternProperties][fmt::format("^description{}$", kPatternConditionConfigurationsPlatforms)] = getDefinition(Defs::TargetDescription);
		targetSource[kPatternProperties][fmt::format("^language{}$", kPatternConditionPlatforms)] = getDefinition(Defs::TargetDescription);
		defs[Defs::LibrarySourceTarget] = std::move(targetSource);

		//
		defs[Defs::ExecutableSourceTarget] = defs[Defs::LibrarySourceTarget];
		defs[Defs::ExecutableSourceTarget][kProperties]["runTarget"] = getDefinition(Defs::TargetRunTarget);
		defs[Defs::ExecutableSourceTarget][kProperties]["runArguments"] = getDefinition(Defs::TargetRunTargetArguments);
		defs[Defs::ExecutableSourceTarget][kProperties]["runDependencies"] = getDefinition(Defs::TargetRunDependencies);
		defs[Defs::ExecutableSourceTarget][kPatternProperties][fmt::format("^runTarget{}$", kPatternConditionConfigurationsPlatforms)] = getDefinition(Defs::TargetRunTarget);
		defs[Defs::ExecutableSourceTarget][kPatternProperties][fmt::format("^runDependencies{}$", kPatternConditionConfigurationsPlatforms)] = getDefinition(Defs::TargetRunDependencies);
	}

	{
		auto targetBuildScript = R"json({
			"type": "object",
			"additionalProperties": false,
			"required": [
				"kind"
			]
		})json"_ojson;
		targetBuildScript[kProperties]["kind"] = getDefinition(Defs::TargetKind);
		targetBuildScript[kProperties]["script"] = getDefinition(Defs::ScriptTargetScript);
		targetBuildScript[kProperties]["description"] = getDefinition(Defs::TargetDescription);
		targetBuildScript[kProperties]["condition"] = getDefinition(Defs::TargetCondition);
		targetBuildScript[kProperties]["runTarget"] = getDefinition(Defs::TargetRunTarget);
		// targetBuildScript[kProperties]["runArguments"] = getDefinition(Defs::TargetRunTargetArguments);
		targetBuildScript[kPatternProperties][fmt::format("^script{}$", kPatternConditionConfigurationsPlatforms)] = getDefinition(Defs::ScriptTargetScript);
		targetBuildScript[kPatternProperties][fmt::format("^description{}$", kPatternConditionConfigurationsPlatforms)] = getDefinition(Defs::TargetDescription);
		targetBuildScript[kPatternProperties][fmt::format("^runTarget{}$", kPatternConditionConfigurationsPlatforms)] = getDefinition(Defs::TargetRunTarget);
		defs[Defs::BuildScriptTarget] = std::move(targetBuildScript);
	}

	{
		auto targetDistScript = R"json({
			"type": "object",
			"additionalProperties": false
		})json"_ojson;
		targetDistScript[kProperties]["kind"] = getDefinition(Defs::DistributionTargetKind);
		targetDistScript[kProperties]["script"] = getDefinition(Defs::ScriptTargetScript);
		targetDistScript[kProperties]["description"] = getDefinition(Defs::TargetDescription);
		targetDistScript[kProperties]["condition"] = getDefinition(Defs::TargetCondition);
		targetDistScript[kPatternProperties][fmt::format("^script{}$", kPatternConditionConfigurationsPlatforms)] = getDefinition(Defs::ScriptTargetScript);
		targetDistScript[kPatternProperties][fmt::format("^description{}$", kPatternConditionConfigurationsPlatforms)] = getDefinition(Defs::TargetDescription);
		defs[Defs::DistScriptTarget] = std::move(targetDistScript);
	}

	{
		auto targetCMake = R"json({
			"type": "object",
			"description": "Build the location with CMake",
			"additionalProperties": false,
			"required": [
				"kind",
				"location"
			]
		})json"_ojson;
		targetCMake[kProperties]["kind"] = getDefinition(Defs::TargetKind);
		targetCMake[kProperties]["description"] = getDefinition(Defs::TargetDescription);
		targetCMake[kProperties]["location"] = getDefinition(Defs::CMakeTargetLocation);
		targetCMake[kProperties]["buildFile"] = getDefinition(Defs::CMakeTargetBuildFile);
		targetCMake[kProperties]["defines"] = getDefinition(Defs::CMakeTargetDefines);
		targetCMake[kProperties]["toolset"] = getDefinition(Defs::CMakeTargetToolset);
		targetCMake[kProperties]["recheck"] = getDefinition(Defs::CMakeTargetRecheck);
		targetCMake[kProperties]["condition"] = getDefinition(Defs::TargetCondition);
		targetCMake[kProperties]["runTarget"] = getDefinition(Defs::TargetRunTarget);
		targetCMake[kProperties]["runArguments"] = getDefinition(Defs::TargetRunTargetArguments);
		targetCMake[kProperties]["runExecutable"] = getDefinition(Defs::CMakeTargetRunExecutable);
		targetCMake[kPatternProperties][fmt::format("^description{}$", kPatternConditionConfigurationsPlatforms)] = getDefinition(Defs::TargetDescription);
		targetCMake[kPatternProperties][fmt::format("^buildFile{}$", kPatternConditionConfigurationsPlatforms)] = getDefinition(Defs::CMakeTargetBuildFile);
		targetCMake[kPatternProperties][fmt::format("^defines{}$", kPatternConditionConfigurationsPlatforms)] = getDefinition(Defs::CMakeTargetDefines);
		targetCMake[kPatternProperties][fmt::format("^toolset{}$", kPatternConditionConfigurationsPlatforms)] = getDefinition(Defs::CMakeTargetToolset);
		targetCMake[kPatternProperties][fmt::format("^runTarget{}$", kPatternConditionConfigurationsPlatforms)] = getDefinition(Defs::TargetRunTarget);
		targetCMake[kPatternProperties][fmt::format("^runExecutable{}$", kPatternConditionConfigurationsPlatforms)] = getDefinition(Defs::CMakeTargetRunExecutable);
		defs[Defs::CMakeTarget] = std::move(targetCMake);
	}

	{
		auto targetChalet = R"json({
			"type": "object",
			"description": "Build the location with Chalet",
			"additionalProperties": false,
			"required": [
				"kind",
				"location"
			]
		})json"_ojson;
		targetChalet[kProperties]["kind"] = getDefinition(Defs::TargetKind);
		targetChalet[kProperties]["description"] = getDefinition(Defs::TargetDescription);
		targetChalet[kProperties]["location"] = getDefinition(Defs::ChaletTargetLocation);
		targetChalet[kProperties]["buildFile"] = getDefinition(Defs::ChaletTargetBuildFile);
		targetChalet[kProperties]["recheck"] = getDefinition(Defs::ChaletTargetRecheck);
		targetChalet[kProperties]["condition"] = getDefinition(Defs::TargetCondition);
		targetChalet[kPatternProperties][fmt::format("^description{}$", kPatternConditionConfigurationsPlatforms)] = getDefinition(Defs::TargetDescription);
		targetChalet[kPatternProperties][fmt::format("^buildFile{}$", kPatternConditionConfigurationsPlatforms)] = getDefinition(Defs::ChaletTargetBuildFile);
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
		case Defs::DistributionTargetKind: return "distribution-target-kind";
		case Defs::DistributionTargetConfiguration: return "distribution-target-configuration";
		case Defs::DistributionTargetInclude: return "distribution-target-include";
		case Defs::DistributionTargetExclude: return "distribution-target-exclude";
		case Defs::DistributionTargetIncludeDependentSharedLibraries: return "distribution-target-includeDependentSharedLibraries";
		case Defs::DistributionTargetLinux: return "distribution-target-linux";
		case Defs::DistributionTargetMacOS: return "distribution-target-macos";
		case Defs::DistributionTargetMainExecutable: return "distribution-target-mainExecutable";
		case Defs::DistributionTargetOutputDirectory: return "distribution-target-subdirectory";
		case Defs::DistributionTargetBuildTargets: return "distribution-target-buildTargets";
		case Defs::DistributionTargetWindows: return "distribution-target-windows";
		//
		case Defs::DistArchiveTarget: return "distribution-archive-target";
		case Defs::DistArchiveTargetBundles: return "distribution-archive-target-bundles";
		//
		case Defs::ExternalDependency: return "external-dependency";
		case Defs::ExternalDependencyGitRepository: return "external-git-repository";
		case Defs::ExternalDependencyGitBranch: return "external-git-branch";
		case Defs::ExternalDependencyGitCommit: return "external-git-commit";
		case Defs::ExternalDependencyGitTag: return "external-git-tag";
		case Defs::ExternalDependencyGitSubmodules: return "external-git-submodules";
		//
		case Defs::EnvironmentSearchPaths: return "environment-searchPaths";
		//
		case Defs::TargetDescription: return "target-description";
		case Defs::TargetKind: return "target-kind";
		case Defs::TargetCondition: return "target-condition";
		case Defs::TargetRunTarget: return "target-runTarget";
		case Defs::TargetRunTargetArguments: return "target-runArguments";
		case Defs::TargetRunDependencies: return "target-runDependencies";
		//
		case Defs::SourceTargetExtends: return "source-target-extends";
		case Defs::SourceTargetFiles: return "source-target-files";
		case Defs::SourceTargetLocation: return "source-target-location";
		case Defs::SourceTargetLanguage: return "source-target-language";
		//
		case Defs::AbstractTarget: return "abstract-target";
		case Defs::ExecutableSourceTarget: return "executable-source-target";
		case Defs::LibrarySourceTarget: return "library-source-target";
		case Defs::SourceTargetCxx: return "source-target-cxx";
		case Defs::SourceTargetCxxCStandard: return "source-target-cxx-cStandard";
		case Defs::SourceTargetCxxCppStandard: return "source-target-cxx-cppStandard";
		case Defs::SourceTargetCxxCompileOptions: return "source-target-cxx-compileOptions";
		case Defs::SourceTargetCxxDefines: return "source-target-cxx-defines";
		case Defs::SourceTargetCxxIncludeDirs: return "source-target-cxx-includeDirs";
		case Defs::SourceTargetCxxLibDirs: return "source-target-cxx-libDirs";
		case Defs::SourceTargetCxxLinkerScript: return "source-target-cxx-linkerScript";
		case Defs::SourceTargetCxxLinkerOptions: return "source-target-cxx-linkerOptions";
		case Defs::SourceTargetCxxLinks: return "source-target-cxx-links";
		case Defs::SourceTargetCxxMacOsFrameworkPaths: return "source-target-cxx-macosFrameworkPaths";
		case Defs::SourceTargetCxxMacOsFrameworks: return "source-target-cxx-macosFrameworks";
		case Defs::SourceTargetCxxPrecompiledHeader: return "source-target-cxx-pch";
		case Defs::SourceTargetCxxThreads: return "source-target-cxx-threads";
		case Defs::SourceTargetCxxCppModules: return "source-target-cxx-cppModules";
		case Defs::SourceTargetCxxCppCoroutines: return "source-target-cxx-cppCoroutines";
		case Defs::SourceTargetCxxCppConcepts: return "source-target-cxx-cppConcepts";
		case Defs::SourceTargetCxxRunTimeTypeInfo: return "source-target-cxx-rtti";
		case Defs::SourceTargetCxxExceptions: return "source-target-cxx-exceptions";
		case Defs::SourceTargetCxxStaticLinking: return "source-target-cxx-staticLinking";
		case Defs::SourceTargetCxxStaticLinks: return "source-target-cxx-staticLinks";
		case Defs::SourceTargetCxxWarnings: return "source-target-cxx-warnings";
		case Defs::SourceTargetCxxWindowsAppManifest: return "source-target-cxx-windowsApplicationManifest";
		case Defs::SourceTargetCxxWindowsAppIcon: return "source-target-cxx-windowsAppIcon";
		// case Defs::SourceTargetCxxWindowsOutputDef: return "source-target-cxx-windowsOutputDef";
		case Defs::SourceTargetCxxWindowsSubSystem: return "source-target-cxx-windowsSubSystem";
		case Defs::SourceTargetCxxWindowsEntryPoint: return "source-target-cxx-windowsEntryPoint";
		//
		case Defs::BuildScriptTarget: return "build-script-target";
		case Defs::DistScriptTarget: return "distribution-script-target";
		case Defs::ScriptTargetScript: return "script-target-script";
		//
		case Defs::CMakeTarget: return "cmake-target";
		case Defs::CMakeTargetLocation: return "cmake-target-location";
		case Defs::CMakeTargetBuildFile: return "cmake-target-buildFile";
		case Defs::CMakeTargetDefines: return "cmake-target-defines";
		case Defs::CMakeTargetRecheck: return "cmake-target-recheck";
		case Defs::CMakeTargetToolset: return "cmake-target-toolset";
		case Defs::CMakeTargetRunExecutable: return "cmake-target-runExecutable";
		//
		case Defs::ChaletTarget: return "chalet-target";
		case Defs::ChaletTargetLocation: return "chalet-target-location";
		case Defs::ChaletTargetBuildFile: return "chalet-target-buildFile";
		case Defs::ChaletTargetRecheck: return "chalet-target-recheck";

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

	ret[kPatternProperties][fmt::format("^abstracts:(\\*|{})$", kPatternAbstractName)] = getDefinition(Defs::AbstractTarget);
	ret[kPatternProperties][fmt::format("^abstracts:(\\*|{})$", kPatternAbstractName)][kDescription] = "An abstract build target. 'abstracts:*' is a special target that gets implicitely added to each project";

	ret[kProperties]["abstracts"] = R"json({
		"type": "object",
		"additionalProperties": false,
		"description": "A list of abstract build targets"
	})json"_ojson;
	ret[kProperties]["abstracts"][kPatternProperties][fmt::format("^(\\*|{})$", kPatternAbstractName)] = getDefinition(Defs::AbstractTarget);
	ret[kProperties]["abstracts"][kPatternProperties][fmt::format("^(\\*|{})$", kPatternAbstractName)][kDescription] = "An abstract build target. '*' is a special target that gets implicitely added to each project.";

	ret[kProperties]["configurations"] = R"json({
		"description": "An array of allowed build configuration presets, or an object of custom build configurations.",
		"default": [],
		"oneOf": [
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
					"minLength": 1
				}
			}
		]
	})json"_ojson;
	ret[kProperties]["configurations"][kOneOf][0][kPatternProperties][R"(^[A-Za-z]{3,}$)"] = getDefinition(Defs::Configuration);
	ret[kProperties]["configurations"][kDefault] = BuildConfiguration::getDefaultBuildConfigurationNames();
	ret[kProperties]["configurations"][kOneOf][1][kItems][kEnum] = BuildConfiguration::getDefaultBuildConfigurationNames();

	ret[kProperties]["distribution"] = R"json({
		"type": "object",
		"additionalProperties": false,
		"description": "A sequential list of distribution targets to be created during the bundle phase."
	})json"_ojson;
	ret[kProperties]["distribution"][kPatternProperties][kPatternDistributionName] = R"json({
		"type": "object",
		"description": "A single distribution target or script.",
		"if": {
			"properties": {
				"kind": { "const": "bundle" }
			}
		},
		"then": {},
		"else": {
			"if": {
				"properties": {
					"kind": { "const": "script" }
				}
			},
			"then": {},
			"else": {
				"if": {
					"properties": {
						"kind": { "const": "archive" }
					}
				},
				"then": {},
				"else": {
					"type": "object",
					"additionalProperties": false
				}
			}
		}
	})json"_ojson;
	ret[kProperties]["distribution"][kPatternProperties][kPatternDistributionName][kThen] = getDefinition(Defs::DistributionTarget);
	ret[kProperties]["distribution"][kPatternProperties][kPatternDistributionName][kElse][kThen] = getDefinition(Defs::DistScriptTarget);
	ret[kProperties]["distribution"][kPatternProperties][kPatternDistributionName][kElse][kElse][kThen] = getDefinition(Defs::DistArchiveTarget);

	ret[kProperties]["externalDependencies"] = R"json({
		"type": "object",
		"additionalProperties": false,
		"description": "A sequential list of externalDependencies to install prior to building or via the configure command. The key will be the destination directory name for the repository within the folder defined by the command-line option 'externalDir'."
	})json"_ojson;
	ret[kProperties]["externalDependencies"][kPatternProperties]["^[\\w\\-+.]{3,100}$"] = getDefinition(Defs::ExternalDependency);

	ret[kProperties]["searchPaths"] = getDefinition(Defs::EnvironmentSearchPaths);
	ret[kPatternProperties][fmt::format("^searchPaths{}$", kPatternConditionConfigurationsPlatforms)] = getDefinition(Defs::EnvironmentSearchPaths);

	const auto targets = "targets";
	ret[kProperties][targets] = R"json({
		"type": "object",
		"additionalProperties": false,
		"description": "A sequential list of build targets, cmake targets, or scripts."
	})json"_ojson;
	ret[kProperties][targets][kPatternProperties][kPatternTargetName] = R"json({
		"type": "object",
		"description": "A single build target or script.",
		"if": {
			"properties": {
				"kind": { "const": "executable" }
			}
		},
		"then": {},
		"else": {
			"if": {
				"properties": {
					"kind": { "enum": [ "staticLibrary", "sharedLibrary" ] }
				}
			},
			"then": {},
			"else": {
				"if": {
					"properties": {
						"kind": { "const": "cmakeProject" }
					}
				},
				"then": {},
				"else": {
					"if": {
						"properties": {
							"kind": { "const": "chaletProject" }
						}
					},
					"then": {},
					"else": {
						"if": {
							"properties": {
								"kind": { "const": "script" }
							}
						},
						"then": {},
						"else": {
							"type": "object",
							"additionalProperties": false
						}
					}
				}
			}
		}
	})json"_ojson;
	ret[kProperties][targets][kPatternProperties][kPatternTargetName][kThen] = getDefinition(Defs::ExecutableSourceTarget);
	ret[kProperties][targets][kPatternProperties][kPatternTargetName][kElse][kThen] = getDefinition(Defs::LibrarySourceTarget);
	ret[kProperties][targets][kPatternProperties][kPatternTargetName][kElse][kElse][kThen] = getDefinition(Defs::CMakeTarget);
	ret[kProperties][targets][kPatternProperties][kPatternTargetName][kElse][kElse][kElse][kThen] = getDefinition(Defs::ChaletTarget);
	ret[kProperties][targets][kPatternProperties][kPatternTargetName][kElse][kElse][kElse][kElse][kThen] = getDefinition(Defs::BuildScriptTarget);
	ret[kProperties][targets][kPatternProperties][kPatternTargetName][kElse][kElse][kElse][kElse][kElse][kProperties]["kind"] = getDefinition(Defs::TargetKind);

	ret[kProperties]["version"] = R"json({
		"type": "string",
		"description": "Version of the workspace project.",
		"minLength": 1,
		"pattern": "^[\\w\\-\\+\\.]+$"
	})json"_ojson;

	ret[kProperties]["workspace"] = R"json({
		"type": "string",
		"description": "The name of the workspace.",
		"minLength": 1,
		"pattern": "^[\\w\\-\\+ ]+$"
	})json"_ojson;

	return ret;
}
}
