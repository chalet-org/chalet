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
	defs[Defs::DistProjects][kItems][kPattern] = kPatternProjectName;

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
	defs[Defs::TargetProjectCxxLinks][kItems][kPattern] = kPatternProjectLinks;

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
	defs[Defs::TargetProjectLocation][kOneOf][2][kPatternProperties][fmt::format("^exclude{}{}$", kPatternConfigurations, kPatternPlatforms)] = R"json({
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
	defs[Defs::TargetProjectLocation][kOneOf][2][kPatternProperties][fmt::format("^include{}{}$", kPatternConfigurations, kPatternPlatforms)] = R"json({
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
	defs[Defs::TargetProjectCxxStaticLinks][kItems][kPattern] = kPatternProjectLinks;

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

	defs[Defs::TargetProjectCxxWarnings][kAnyOf][1][kItems][kExamples] = {
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

	//
	// Complex Definitions
	//
	{

		auto configuration = R"json({
			"type": "object",
			"additionalProperties": false
		})json"_ojson;
		configuration[kProperties]["debugSymbols"] = getDefinition(Defs::ConfigDebugSymbols);
		configuration[kProperties]["enableProfiling"] = getDefinition(Defs::ConfigEnableProfiling);
		configuration[kProperties]["linkTimeOptimization"] = getDefinition(Defs::ConfigLinkTimeOptimizations);
		configuration[kProperties]["optimizations"] = getDefinition(Defs::ConfigOptimizationLevel);
		configuration[kProperties]["stripSymbols"] = getDefinition(Defs::ConfigStripSymbols);
		defs[Defs::Configuration] = std::move(configuration);
	}

	{
		auto distDef = R"json({
			"type": "object",
			"additionalProperties": false,
			"description": "Variables to describe the final output build."
		})json"_ojson;
		distDef[kProperties] = Json::object();
		distDef[kProperties]["configuration"] = getDefinition(Defs::DistConfiguration);
		distDef[kProperties]["dependencies"] = getDefinition(Defs::DistDependencies);
		distDef[kProperties]["description"] = getDefinition(Defs::DistDescription);
		distDef[kProperties]["exclude"] = getDefinition(Defs::DistExclude);
		distDef[kProperties]["includeDependentSharedLibraries"] = getDefinition(Defs::DistIncludeDependentSharedLibs);
		distDef[kProperties]["linux"] = getDefinition(Defs::DistLinux);
		distDef[kProperties]["macos"] = getDefinition(Defs::DistMacOS);
		distDef[kProperties]["windows"] = getDefinition(Defs::DistWindows);
		distDef[kProperties]["mainProject"] = getDefinition(Defs::DistMainProject);
		distDef[kProperties]["outDir"] = getDefinition(Defs::DistOutDirectory);
		distDef[kProperties]["projects"] = getDefinition(Defs::DistProjects);
		distDef[kPatternProperties][fmt::format("^dependencies{}{}$", kPatternConfigurations, kPatternPlatforms)] = getDefinition(Defs::DistDependencies);
		distDef[kPatternProperties][fmt::format("^exclude{}{}$", kPatternConfigurations, kPatternPlatforms)] = getDefinition(Defs::DistExclude);
		defs[Defs::Dist] = std::move(distDef);
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
		externalDependency[kOneOf][0][kProperties]["repository"] = getDefinition(Defs::ExtGitRepository);
		externalDependency[kOneOf][0][kProperties]["submodules"] = getDefinition(Defs::ExtGitSubmodules);
		externalDependency[kOneOf][0][kProperties]["tag"] = getDefinition(Defs::ExtGitTag);

		externalDependency[kOneOf][1][kProperties] = Json::object();
		externalDependency[kOneOf][1][kProperties]["repository"] = getDefinition(Defs::ExtGitRepository);
		externalDependency[kOneOf][1][kProperties]["submodules"] = getDefinition(Defs::ExtGitSubmodules);
		externalDependency[kOneOf][1][kProperties]["branch"] = getDefinition(Defs::ExtGitBranch);
		externalDependency[kOneOf][1][kProperties]["commit"] = getDefinition(Defs::ExtGitCommit);
		defs[Defs::ExternalDependency] = std::move(externalDependency);
	}

	{
		auto targetProjectCxx = R"json({
			"type": "object",
			"additionalProperties": false
		})json"_ojson;
		targetProjectCxx[kProperties]["cStandard"] = getDefinition(Defs::TargetProjectCxxCStandard);
		targetProjectCxx[kProperties]["compileOptions"] = getDefinition(Defs::TargetProjectCxxCompileOptions);
		targetProjectCxx[kProperties]["cppStandard"] = getDefinition(Defs::TargetProjectCxxCppStandard);
		targetProjectCxx[kProperties]["defines"] = getDefinition(Defs::TargetProjectCxxDefines);
		targetProjectCxx[kProperties]["includeDirs"] = getDefinition(Defs::TargetProjectCxxIncludeDirs);
		targetProjectCxx[kProperties]["libDirs"] = getDefinition(Defs::TargetProjectCxxLibDirs);
		targetProjectCxx[kProperties]["linkerScript"] = getDefinition(Defs::TargetProjectCxxLinkerScript);
		targetProjectCxx[kProperties]["linkerOptions"] = getDefinition(Defs::TargetProjectCxxLinkerOptions);
		targetProjectCxx[kProperties]["links"] = getDefinition(Defs::TargetProjectCxxLinks);
		targetProjectCxx[kProperties]["macosFrameworkPaths"] = getDefinition(Defs::TargetProjectCxxMacOsFrameworkPaths);
		targetProjectCxx[kProperties]["macosFrameworks"] = getDefinition(Defs::TargetProjectCxxMacOsFrameworks);
		targetProjectCxx[kProperties]["objectiveCxx"] = getDefinition(Defs::TargetProjectCxxObjectiveCxx);
		targetProjectCxx[kProperties]["pch"] = getDefinition(Defs::TargetProjectCxxPrecompiledHeader);
		targetProjectCxx[kProperties]["threads"] = getDefinition(Defs::TargetProjectCxxThreads);
		targetProjectCxx[kProperties]["rtti"] = getDefinition(Defs::TargetProjectCxxRunTimeTypeInfo);
		targetProjectCxx[kProperties]["exceptions"] = getDefinition(Defs::TargetProjectCxxExceptions);
		targetProjectCxx[kProperties]["staticLinking"] = getDefinition(Defs::TargetProjectCxxStaticLinking);
		targetProjectCxx[kProperties]["staticLinks"] = getDefinition(Defs::TargetProjectCxxStaticLinks);
		targetProjectCxx[kProperties]["warnings"] = getDefinition(Defs::TargetProjectCxxWarnings);
		targetProjectCxx[kProperties]["windowsPrefixOutputFilename"] = getDefinition(Defs::TargetProjectCxxWindowsPrefixOutputFilename);
		targetProjectCxx[kProperties]["windowsOutputDef"] = getDefinition(Defs::TargetProjectCxxWindowsOutputDef);
		targetProjectCxx[kProperties]["windowsApplicationIcon"] = getDefinition(Defs::TargetProjectCxxWindowsAppIcon);
		targetProjectCxx[kProperties]["windowsApplicationManifest"] = getDefinition(Defs::TargetProjectCxxWindowsAppManifest);

		targetProjectCxx[kPatternProperties][fmt::format("^cStandard{}$", kPatternPlatforms)] = getDefinition(Defs::TargetProjectCxxCStandard);
		targetProjectCxx[kPatternProperties][fmt::format("^cppStandard{}$", kPatternPlatforms)] = getDefinition(Defs::TargetProjectCxxCppStandard);
		targetProjectCxx[kPatternProperties][fmt::format("^compileOptions{}$", kPatternPlatforms)] = getDefinition(Defs::TargetProjectCxxCompileOptions);
		targetProjectCxx[kPatternProperties][fmt::format("^defines{}{}$", kPatternConfigurations, kPatternPlatforms)] = getDefinition(Defs::TargetProjectCxxDefines);
		targetProjectCxx[kPatternProperties][fmt::format("^includeDirs{}$", kPatternPlatforms)] = getDefinition(Defs::TargetProjectCxxIncludeDirs);
		targetProjectCxx[kPatternProperties][fmt::format("^libDirs{}$", kPatternPlatforms)] = getDefinition(Defs::TargetProjectCxxLibDirs);
		targetProjectCxx[kPatternProperties][fmt::format("^linkerScript{}$", kPatternPlatforms)] = getDefinition(Defs::TargetProjectCxxLinkerScript);
		targetProjectCxx[kPatternProperties][fmt::format("^linkerOptions{}$", kPatternPlatforms)] = getDefinition(Defs::TargetProjectCxxLinkerOptions);
		targetProjectCxx[kPatternProperties][fmt::format("^links{}{}$", kPatternConfigurations, kPatternPlatforms)] = getDefinition(Defs::TargetProjectCxxLinks);
		targetProjectCxx[kPatternProperties][fmt::format("^objectiveCxx{}$", kPatternPlatforms)] = getDefinition(Defs::TargetProjectCxxObjectiveCxx);
		targetProjectCxx[kPatternProperties][fmt::format("^staticLinks{}{}$", kPatternConfigurations, kPatternPlatforms)] = getDefinition(Defs::TargetProjectCxxStaticLinks);
		targetProjectCxx[kPatternProperties][fmt::format("^threads{}{}$", kPatternConfigurations, kPatternPlatforms)] = getDefinition(Defs::TargetProjectCxxThreads);
		targetProjectCxx[kPatternProperties][fmt::format("^rtti{}{}$", kPatternConfigurations, kPatternPlatforms)] = getDefinition(Defs::TargetProjectCxxRunTimeTypeInfo);
		targetProjectCxx[kPatternProperties][fmt::format("^exceptions{}{}$", kPatternConfigurations, kPatternPlatforms)] = getDefinition(Defs::TargetProjectCxxExceptions);
		targetProjectCxx[kPatternProperties][fmt::format("^staticLinking{}{}$", kPatternConfigurations, kPatternPlatforms)] = getDefinition(Defs::TargetProjectCxxStaticLinking);
		defs[Defs::TargetProjectCxx] = std::move(targetProjectCxx);
	}

	{
		auto targetProject = R"json({
			"type": "object",
			"additionalProperties": false
		})json"_ojson;
		targetProject[kProperties]["settings:Cxx"] = getDefinition(Defs::TargetProjectCxx);
		targetProject[kProperties]["extends"] = getDefinition(Defs::TargetProjectExtends);
		targetProject[kProperties]["files"] = getDefinition(Defs::TargetProjectFiles);
		targetProject[kProperties]["kind"] = getDefinition(Defs::TargetProjectKind);
		targetProject[kProperties]["language"] = getDefinition(Defs::TargetProjectLanguage);
		targetProject[kProperties]["location"] = getDefinition(Defs::TargetProjectLocation);
		targetProject[kProperties]["onlyInConfiguration"] = getDefinition(Defs::TargetOnlyInConfiguration);
		targetProject[kProperties]["notInConfiguration"] = getDefinition(Defs::TargetNotInConfiguration);
		targetProject[kProperties]["onlyInPlatform"] = getDefinition(Defs::TargetOnlyInPlatform);
		targetProject[kProperties]["notInPlatform"] = getDefinition(Defs::TargetNotInPlatform);
		targetProject[kProperties]["runProject"] = getDefinition(Defs::TargetProjectRunProject);
		targetProject[kProperties]["runArguments"] = getDefinition(Defs::TargetProjectRunArguments);
		targetProject[kProperties]["runDependencies"] = getDefinition(Defs::TargetProjectRunDependencies);
		targetProject[kPatternProperties][fmt::format("^runProject{}{}$", kPatternConfigurations, kPatternPlatforms)] = getDefinition(Defs::TargetProjectRunProject);
		targetProject[kPatternProperties][fmt::format("^runDependencies{}{}$", kPatternConfigurations, kPatternPlatforms)] = getDefinition(Defs::TargetProjectRunDependencies);
		defs[Defs::TargetProject] = std::move(targetProject);
	}

	{
		auto targetScript = R"json({
			"type": "object",
			"additionalProperties": false
		})json"_ojson;
		targetScript[kProperties]["script"] = getDefinition(Defs::TargetScriptScript);
		targetScript[kProperties]["script"][kDescription] = "Script(s) to run during this build step.";
		targetScript[kProperties]["description"] = getDefinition(Defs::TargetDescription);
		{
			auto scriptPattern = fmt::format("^script{}{}$", kPatternConfigurations, kPatternPlatforms);
			targetScript[kPatternProperties][scriptPattern] = getDefinition(Defs::TargetScriptScript);
			targetScript[kPatternProperties][scriptPattern][kDescription] = "Script(s) to run during this build step.";
		}
		targetScript[kPatternProperties][fmt::format("^description{}{}$", kPatternConfigurations, kPatternPlatforms)] = getDefinition(Defs::TargetDescription);
		defs[Defs::TargetScript] = std::move(targetScript);
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
		targetCMake[kProperties]["location"] = getDefinition(Defs::TargetCMakeLocation);
		targetCMake[kProperties]["buildFile"] = getDefinition(Defs::TargetCMakeBuildFile);
		targetCMake[kProperties]["defines"] = getDefinition(Defs::TargetCMakeDefines);
		targetCMake[kProperties]["toolset"] = getDefinition(Defs::TargetCMakeToolset);
		targetCMake[kProperties]["recheck"] = getDefinition(Defs::TargetCMakeRecheck);
		targetCMake[kProperties]["type"] = getDefinition(Defs::TargetType);
		targetCMake[kProperties]["onlyInConfiguration"] = getDefinition(Defs::TargetOnlyInConfiguration);
		targetCMake[kProperties]["notInConfiguration"] = getDefinition(Defs::TargetNotInConfiguration);
		targetCMake[kProperties]["onlyInPlatform"] = getDefinition(Defs::TargetOnlyInPlatform);
		targetCMake[kProperties]["notInPlatform"] = getDefinition(Defs::TargetNotInPlatform);
		targetCMake[kPatternProperties][fmt::format("^description{}{}$", kPatternConfigurations, kPatternPlatforms)] = getDefinition(Defs::TargetDescription);
		targetCMake[kPatternProperties][fmt::format("^buildFile{}{}$", kPatternConfigurations, kPatternPlatforms)] = getDefinition(Defs::TargetCMakeBuildFile);
		targetCMake[kPatternProperties][fmt::format("^defines{}{}$", kPatternConfigurations, kPatternPlatforms)] = getDefinition(Defs::TargetCMakeDefines);
		targetCMake[kPatternProperties][fmt::format("^toolset{}{}$", kPatternConfigurations, kPatternPlatforms)] = getDefinition(Defs::TargetCMakeToolset);
		defs[Defs::TargetCMake] = std::move(targetCMake);
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
		targetChalet[kProperties]["location"] = getDefinition(Defs::TargetChaletLocation);
		targetChalet[kProperties]["buildFile"] = getDefinition(Defs::TargetChaletBuildFile);
		targetChalet[kProperties]["recheck"] = getDefinition(Defs::TargetChaletRecheck);
		targetChalet[kProperties]["type"] = getDefinition(Defs::TargetType);
		targetChalet[kProperties]["onlyInConfiguration"] = getDefinition(Defs::TargetOnlyInConfiguration);
		targetChalet[kProperties]["notInConfiguration"] = getDefinition(Defs::TargetNotInConfiguration);
		targetChalet[kProperties]["onlyInPlatform"] = getDefinition(Defs::TargetOnlyInPlatform);
		targetChalet[kProperties]["notInPlatform"] = getDefinition(Defs::TargetNotInPlatform);
		targetChalet[kPatternProperties][fmt::format("^description{}{}$", kPatternConfigurations, kPatternPlatforms)] = getDefinition(Defs::TargetDescription);
		targetChalet[kPatternProperties][fmt::format("^buildFile{}{}$", kPatternConfigurations, kPatternPlatforms)] = getDefinition(Defs::TargetChaletBuildFile);
		defs[Defs::TargetChalet] = std::move(targetChalet);
	}

	return defs;
}

/*****************************************************************************/
std::string SchemaBuildJson::getDefinitionName(const Defs inDef)
{
	switch (inDef)
	{
		case Defs::Configuration: return "configuration";
		case Defs::ConfigDebugSymbols: return "config-debugSymbols";
		case Defs::ConfigEnableProfiling: return "config-enableProfiling";
		case Defs::ConfigLinkTimeOptimizations: return "config-linkTimeOptimizations";
		case Defs::ConfigOptimizationLevel: return "config-optimizationLevel";
		case Defs::ConfigStripSymbols: return "config-stripSymbols";
		//
		case Defs::Dist: return "distribution";
		case Defs::DistConfiguration: return "dist-configuration";
		case Defs::DistDependencies: return "dist-dependencies";
		case Defs::DistDescription: return "dist-description";
		case Defs::DistExclude: return "dist-exclude";
		case Defs::DistIncludeDependentSharedLibs: return "dist-includeDependentSharedLibs";
		case Defs::DistLinux: return "dist-linux";
		case Defs::DistMacOS: return "dist-macos";
		case Defs::DistMainProject: return "dist-mainProject";
		case Defs::DistOutDirectory: return "dist-outDir";
		case Defs::DistProjects: return "dist-projects";
		case Defs::DistWindows: return "dist-windows";
		//
		case Defs::ExternalDependency: return "external-dependency";
		case Defs::ExtGitRepository: return "external-git-repository";
		case Defs::ExtGitBranch: return "external-git-branch";
		case Defs::ExtGitCommit: return "external-git-commit";
		case Defs::ExtGitTag: return "external-git-tag";
		case Defs::ExtGitSubmodules: return "external-git-submodules";
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
		case Defs::TargetProjectExtends: return "target-project-extends";
		case Defs::TargetProjectFiles: return "target-project-files";
		case Defs::TargetProjectKind: return "target-project-kind";
		case Defs::TargetProjectLocation: return "target-project-location";
		case Defs::TargetProjectLanguage: return "target-project-language";
		case Defs::TargetProjectRunProject: return "target-project-runProject";
		case Defs::TargetProjectRunArguments: return "target-project-runArguments";
		case Defs::TargetProjectRunDependencies: return "target-project-runDependencies";
		//
		case Defs::TargetProject: return "target-project";
		case Defs::TargetProjectCxx: return "target-project-cxx";
		case Defs::TargetProjectCxxCStandard: return "target-project-cxx-cStandard";
		case Defs::TargetProjectCxxCppStandard: return "target-project-cxx-cppStandard";
		case Defs::TargetProjectCxxCompileOptions: return "target-project-cxx-compileOptions";
		case Defs::TargetProjectCxxDefines: return "target-project-cxx-defines";
		case Defs::TargetProjectCxxIncludeDirs: return "target-project-cxx-includeDirs";
		case Defs::TargetProjectCxxLibDirs: return "target-project-cxx-libDirs";
		case Defs::TargetProjectCxxLinkerScript: return "target-project-cxx-linkerScript";
		case Defs::TargetProjectCxxLinkerOptions: return "target-project-cxx-linkerOptions";
		case Defs::TargetProjectCxxLinks: return "target-project-cxx-links";
		case Defs::TargetProjectCxxMacOsFrameworkPaths: return "target-project-cxx-macosFrameworkPaths";
		case Defs::TargetProjectCxxMacOsFrameworks: return "target-project-cxx-macosFrameworks";
		case Defs::TargetProjectCxxObjectiveCxx: return "target-project-cxx-objectiveCxx";
		case Defs::TargetProjectCxxPrecompiledHeader: return "target-project-cxx-pch";
		case Defs::TargetProjectCxxThreads: return "target-project-cxx-threads";
		case Defs::TargetProjectCxxRunTimeTypeInfo: return "target-project-cxx-rtti";
		case Defs::TargetProjectCxxExceptions: return "target-project-cxx-exceptions";
		case Defs::TargetProjectCxxStaticLinking: return "target-project-cxx-staticLinking";
		case Defs::TargetProjectCxxStaticLinks: return "target-project-cxx-staticLinks";
		case Defs::TargetProjectCxxWarnings: return "target-project-cxx-warnings";
		case Defs::TargetProjectCxxWindowsAppManifest: return "target-project-cxx-windowsAppManifest";
		case Defs::TargetProjectCxxWindowsAppIcon: return "target-project-cxx-windowsAppIcon";
		case Defs::TargetProjectCxxWindowsOutputDef: return "target-project-cxx-windowsOutputDef";
		case Defs::TargetProjectCxxWindowsPrefixOutputFilename: return "target-project-cxx-windowsPrefixOutputFilename";
		//
		case Defs::TargetScript: return "target-script";
		case Defs::TargetScriptScript: return "target-script-script";
		//
		case Defs::TargetCMake: return "target-cmake";
		case Defs::TargetCMakeLocation: return "target-cmake-location";
		case Defs::TargetCMakeBuildFile: return "target-cmake-buildFile";
		case Defs::TargetCMakeDefines: return "target-cmake-defines";
		case Defs::TargetCMakeRecheck: return "target-cmake-recheck";
		case Defs::TargetCMakeToolset: return "target-cmake-toolset";
		//
		case Defs::TargetChalet: return "target-chalet";
		case Defs::TargetChaletLocation: return "target-chalet-location";
		case Defs::TargetChaletBuildFile: return "target-chalet-buildFile";
		case Defs::TargetChaletRecheck: return "target-chalet-recheck";

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

	ret[kPatternProperties]["^abstracts:[a-z]+$"] = getDefinition(Defs::TargetProject);
	ret[kPatternProperties]["^abstracts:[a-z]+$"][kDescription] = "An abstract build project. 'abstracts:all' is a special project that gets implicitely added to each project";

	ret[kProperties]["abstracts"] = R"json({
		"type": "object",
		"additionalProperties": false,
		"description": "A list of abstract build projects"
	})json"_ojson;
	ret[kProperties]["abstracts"][kPatternProperties][R"(^[A-Za-z_-]+$)"] = getDefinition(Defs::TargetProject);
	ret[kProperties]["abstracts"][kPatternProperties][R"(^[A-Za-z_-]+$)"][kDescription] = "An abstract build project. 'all' is implicitely added to each project.";

	ret[kProperties]["distribution"] = R"json({
		"type": "object",
		"additionalProperties": false,
		"description": "A list of bundle descriptors for the distribution."
	})json"_ojson;
	ret[kProperties]["distribution"][kPatternProperties][kPatternDistributionName] = R"json({
		"description": "A single bundle or script."
	})json"_ojson;
	ret[kProperties]["distribution"][kPatternProperties][kPatternDistributionName][kOneOf][0] = getDefinition(Defs::TargetScript);
	ret[kProperties]["distribution"][kPatternProperties][kPatternDistributionName][kOneOf][1] = getDefinition(Defs::Dist);

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
	ret[kProperties]["configurations"][kAnyOf][0][kPatternProperties][R"(^[A-Za-z]{3,}$)"] = getDefinition(Defs::Configuration);

	ret[kProperties]["externalDependencies"] = R"json({
		"type": "object",
		"additionalProperties": false,
		"description": "A sequential list of externalDependencies to install prior to building or via the configure command. The key will be the destination directory name for the repository within the folder defined in 'externalDepDir'."
	})json"_ojson;
	ret[kProperties]["externalDependencies"][kPatternProperties]["^[\\w\\-\\+\\.]{3,100}$"] = getDefinition(Defs::ExternalDependency);

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
	ret[kProperties][targets][kPatternProperties][kPatternProjectName][kOneOf][0] = getDefinition(Defs::TargetProject);
	ret[kProperties][targets][kPatternProperties][kPatternProjectName][kOneOf][1] = getDefinition(Defs::TargetScript);
	ret[kProperties][targets][kPatternProperties][kPatternProjectName][kOneOf][2][kProperties]["type"] = getDefinition(Defs::TargetType);
	ret[kProperties][targets][kPatternProperties][kPatternProjectName][kOneOf][2][kAllOf][0]["then"] = getDefinition(Defs::TargetCMake);
	ret[kProperties][targets][kPatternProperties][kPatternProjectName][kOneOf][2][kAllOf][1]["then"] = getDefinition(Defs::TargetChalet);

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

	ret[kProperties]["path"] = getDefinition(Defs::EnvironmentPath);
	ret[kPatternProperties][fmt::format("^path{}{}$", kPatternConfigurations, kPatternPlatforms)] = getDefinition(Defs::EnvironmentPath);

	return ret;
}
}
