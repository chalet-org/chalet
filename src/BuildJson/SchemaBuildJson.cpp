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
	kPatternTargetSourceLinks(R"(^[\w\-+.]+$)"),
	kPatternDistributionName(R"(^(([\w\-+. ()]+)|(\$\{(targetTriple|toolchainName|configuration|architecture|buildDir)\}))+$)"),
	kPatternDistributionNameSimple(R"(^[\w\-+. ()]{2,}$)"),
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

	defs[Defs::ConfigurationSanitize] = R"json({
		"type": "array",
		"description": "An array of sanitizers to enable. If combined with staticLinking, the selected sanitizers will be statically linked, if available by the toolchain.",
		"uniqueItems": true,
		"minItems": 1,
		"items": {
			"type": "string",
			"minLength": 1,
			"enum": [
				"address",
				"hwaddress",
				"thread",
				"memory",
				"leak",
				"undefined"
			]
		}
	})json"_ojson;

	//
	// distribution
	//
	defs[Defs::DistributionKind] = R"json({
		"type": "string",
		"description": "Whether the distribution target is a bundle, script, archive, or something platform-specific.",
		"minLength": 1,
		"enum": [
			"bundle",
			"script",
			"archive",
			"macosDiskImage",
			"windowsNullsoftInstaller"
		]
	})json"_ojson;

	defs[Defs::DistributionConfiguration] = R"json({
		"type": "string",
		"description": "The name of the build configuration to use.\nIf this property is omitted, the 'Release' configuration will be used. In the case where custom configurations are defined, the first configuration without 'debugSymbols' and 'enableProfiling' is used.",
		"minLength": 1,
		"default": "Release"
	})json"_ojson;

	defs[Defs::DistributionBundleInclude] = R"json({
		"type": "array",
		"description": "A list of files or folders to copy into the output directory of the bundle.\nIn MacOS, these will be placed into the 'Resources' folder of the application bundle.",
		"uniqueItems": true,
		"minItems": 1,
		"items": {
			"type": "string",
			"description": "A single file or folder to copy.",
			"minLength": 1
		}
	})json"_ojson;

	defs[Defs::DistributionBundleExclude] = R"json({
		"type": "array",
		"description": "In folder paths that are included with 'include', exclude certain files or paths.\nCan accept a glob pattern.",
		"uniqueItems": true,
		"minItems": 1,
		"items": {
			"type": "string"
		}
	})json"_ojson;

	defs[Defs::DistributionBundleIncludeDependentSharedLibraries] = R"json({
		"type": "boolean",
		"description": "If true (default), any shared libraries that the bundle depeends on will also be copied.",
		"default": true
	})json"_ojson;

	{
		auto macosBundleType = R"json({
			"type": "string",
			"description": "The MacOS bundle type (only .app is supported currently)",
			"minLength": 1,
			"enum": [
				"app"
			],
			"default": "app"
		})json"_ojson;

		auto macosBundleIcon = R"json({
			"type": "string",
			"description": "The path to a MacOS bundle icon either in PNG or ICNS format.\nIf the file is a .png, it will get converted to .icns during the bundle process.",
			"minLength": 1,
			"default": "icon.png"
		})json"_ojson;

		auto macosInfoPropertyList = R"json({
			"description": "The path to a property list (.plist) file, .json file, or the properties themselves to export as a plist defining the bundle.",
			"oneOf": [
				{
					"type": "string",
					"minLength": 1,
					"default": "Info.plist.json"
				},
				{
					"type": "object"
				}
			]
		})json"_ojson;
		macosInfoPropertyList[kOneOf][1]["default"] = JsonComments::parseLiteral(PlatformFileTemplates::macosInfoPlist());

		m_nonIndexedDefs[Defs::DistributionBundleMacOSBundle] = R"json({
			"type": "object",
			"description": "Properties to describe the MacOS bundle.",
			"additionalProperties": false,
			"required": [
				"type",
				"infoPropertyList"
			],
			"properties": {}
		})json"_ojson;
		m_nonIndexedDefs[Defs::DistributionBundleMacOSBundle][kProperties]["type"] = std::move(macosBundleType);
		m_nonIndexedDefs[Defs::DistributionBundleMacOSBundle][kProperties]["icon"] = std::move(macosBundleIcon);
		m_nonIndexedDefs[Defs::DistributionBundleMacOSBundle][kProperties]["infoPropertyList"] = std::move(macosInfoPropertyList);
	}

	{
		auto linuxDesktopEntryTemplate = R"json({
			"type": "string",
			"description": "The location to an XDG Desktop Entry template. If the file does not exist, a basic one will be generated in its place.",
			"minLength": 1,
			"default": "app.desktop"
		})json"_ojson;

		auto linuxDesktopEntryIcon = R"json({
			"type": "string",
			"description": "The location to an icon to use with the XDG Desktop Entry (PNG 256x256 is recommended)",
			"minLength": 1,
			"default": "icon.png"
		})json"_ojson;

		m_nonIndexedDefs[Defs::DistributionBundleLinuxDesktopEntry] = R"json({
			"type": "object",
			"description": "Properties to describe an XDG Desktop Entry.",
			"additionalProperties": false,
			"required": [
				"template"
			],
			"properties": {}
		})json"_ojson;
		m_nonIndexedDefs[Defs::DistributionBundleLinuxDesktopEntry][kProperties]["template"] = std::move(linuxDesktopEntryTemplate);
		m_nonIndexedDefs[Defs::DistributionBundleLinuxDesktopEntry][kProperties]["icon"] = std::move(linuxDesktopEntryIcon);
	}

	defs[Defs::DistributionBundleMainExecutable] = R"json({
		"type": "string",
		"description": "The name of the main executable project target.\nIf this property is not defined, the first executable in the 'buildTargets' array of the bundle will be chosen as the main executable.",
		"minLength": 1
	})json"_ojson;

	defs[Defs::DistributionBundleOutputDirectory] = R"json({
		"type": "string",
		"description": "The output folder to place the final build along with all of its included resources and shared libraries.",
		"minLength": 1,
		"default": "dist"
	})json"_ojson;

	defs[Defs::DistributionBundleBuildTargets] = R"json({
		"type": "array",
		"uniqueItems": true,
		"description": "An array of build target names to include in this bundle.\nIf 'mainExecutable' is not defined, the first executable target in this list will be chosen as the main exectuable.",
		"minItems": 1,
		"items": {
			"type": "string",
			"description": "The name of the build target.",
			"minLength": 1
		}
	})json"_ojson;
	defs[Defs::DistributionBundleBuildTargets][kItems][kPattern] = kPatternTargetName;

	//
	defs[Defs::DistributionArchiveInclude] = R"json({
		"description": "A list of files or folders to add to the archive, relative to the root distribution directory. Glob patterns are also accepted. '*' will archive everything in the bundle directory.",
		"oneOf": [
			{
				"type": "string",
				"minLength": 1,
				"default": "*"
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

	//
	defs[Defs::DistributionMacosDiskImageIconSize] = R"json({
		"type": "integer",
		"description": "The icon size in the root of the disk image.",
		"default": 48,
		"minimum": 16,
		"maximum": 512
	})json"_ojson;

	//
	defs[Defs::DistributionMacosDiskImageTextSize] = R"json({
		"type": "integer",
		"description": "The text size in the root of the disk image.",
		"default": 12,
		"minimum": 10,
		"maximum": 16
	})json"_ojson;

	defs[Defs::DistributionMacosDiskImagePathbarVisible] = R"json({
		"type": "boolean",
		"description": "true to display the pathbar (aka breadcrumbs) in the root of the disk image. false to hide it.",
		"default": false
	})json"_ojson;

	defs[Defs::DistributionMacosDiskImageBackground] = R"json({
		"description": "Either a path to a TIFF, a PNG background image, or paths to 1x/2x PNG background images.",
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
	})json"_ojson;

	m_nonIndexedDefs[Defs::DistributionMacosDiskImageSize] = R"json({
		"type": "object",
		"description": "The dimensions of the root of the disk image.",
		"additionalProperties": false,
		"required": [
			"width",
			"height"
		],
		"properties": {
			"width": {
				"type": "integer",
				"description": "The width of the disk image",
				"default": 512,
				"minimum": 128,
				"maximum": 32000
			},
			"height": {
				"type": "integer",
				"description": "The height of the disk image",
				"default": 342,
				"minimum": 128,
				"maximum": 32000
			}
		}
	})json"_ojson;

	m_nonIndexedDefs[Defs::DistributionMacosDiskImagePositions] = R"json({
		"type": "object",
		"description": "The icon positions of paths in the root disk image. Specifying the name of a bundle will include it in the image. Specifying 'Applications' will include a symlink to the '/Applications' path. Additionally, if there is a bundle named 'Applications', it will be ignored, and an error will be displayed.",
		"additionalProperties": false
	})json"_ojson;
	m_nonIndexedDefs[Defs::DistributionMacosDiskImagePositions][kPatternProperties][kPatternDistributionNameSimple] = R"json({
		"type": "object",
		"description": "An icon position in the root disk image.",
		"additionalProperties": false,
		"required": [
			"x",
			"y"
		],
		"properties": {
			"x": {
				"type": "integer",
				"description": "The x position of the path's icon",
				"default": 80,
				"minimum": -1024,
				"maximum": 32000
			},
			"y": {
				"type": "integer",
				"description": "The x position of the path's icon",
				"default": 80,
				"minimum": -1024,
				"maximum": 32000
			}
		}
	})json"_ojson;

	//
	defs[Defs::DistributionWindowsNullsoftInstallerScript] = R"json({
		"type": "string",
		"description": "Relative path to an NSIS installer script (.nsi) to compile.",
		"minLength": 1
	})json"_ojson;

	defs[Defs::DistributionWindowsNullsoftInstallerPluginDirs] = R"json({
		"type": "array",
		"description": "Relative paths to additional NSIS plugin folders. Can accept a root path that contains standard NSIS plugin path structures like 'Plugins/x86-unicode' and 'x86-unicode'",
		"uniqueItems": true,
		"minItems": 1,
		"items": {
			"type": "string",
			"minLength": 1
		}
	})json"_ojson;

	defs[Defs::DistributionWindowsNullsoftInstallerDefines] = R"json({
		"type": "array",
		"description": "A list of defines to pass to MakeNSIS during the build of the installer.",
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

	defs[Defs::DistributionCondition] = R"json({
		"type": "string",
		"description": "A rule describing when to include this target in the distribution.",
		"minLength": 1
	})json"_ojson;
	defs[Defs::DistributionCondition][kPattern] = fmt::format("^{}$", kPatternConditionPlatformsInner);

	defs[Defs::TargetSourceExtends] = R"json({
		"type": "string",
		"description": "A project template to extend. Defaults to '*' implicitly.",
		"pattern": "",
		"minLength": 1,
		"default": "*"
	})json"_ojson;
	defs[Defs::TargetSourceExtends][kPattern] = fmt::format("^{}$", kPatternAbstractName);

	defs[Defs::TargetSourceFiles] = R"json({
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

	defs[Defs::TargetSourceLanguage] = R"json({
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

	defs[Defs::TargetSourceCxxCStandard] = R"json({
		"type": "string",
		"description": "The C standard to use in the compilation",
		"pattern": "^((c|gnu)\\d[\\dx]|(iso9899:(1990|199409|1999|199x|20\\d{2})))$",
		"minLength": 1,
		"default": "c11"
	})json"_ojson;

	{
		m_nonIndexedDefs[Defs::TargetSourceCxxCompileOptions] = R"json({
			"type": "object",
			"additionalProperties": false,
			"description": "Addtional options (per compiler type) to add during the compilation step."
		})json"_ojson;
		m_nonIndexedDefs[Defs::TargetSourceCxxCompileOptions][kPatternProperties][kPatternCompilers] = R"json({
			"type": "string",
			"minLength": 1
		})json"_ojson;
	}

	defs[Defs::TargetSourceCxxCppStandard] = R"json({
		"type": "string",
		"description": "The C++ standard to use in the compilation",
		"pattern": "^(c|gnu)\\+\\+\\d[\\dxyzab]$",
		"minLength": 1,
		"default": "c++17"
	})json"_ojson;

	defs[Defs::TargetSourceCxxDefines] = R"json({
		"type": "array",
		"uniqueItems": true,
		"minItems": 1,
		"description": "Macro definitions to be used by the preprocessor",
		"items": {
			"type": "string",
			"minLength": 1
		}
	})json"_ojson;

	defs[Defs::TargetSourceCxxIncludeDirs] = R"json({
		"type": "array",
		"uniqueItems": true,
		"minItems": 1,
		"description": "A list of directories to include with the project.",
		"items": {
			"type": "string",
			"minLength": 1
		}
	})json"_ojson;

	defs[Defs::TargetSourceCxxLibDirs] = R"json({
		"type": "array",
		"uniqueItems": true,
		"minItems": 1,
		"description": "Fallback search paths to look for static or dynamic libraries (/usr/lib is included by default)",
		"items": {
			"type": "string",
			"minLength": 1
		}
	})json"_ojson;

	defs[Defs::TargetSourceCxxLinkerScript] = R"json({
		"type": "string",
		"description": "An LD linker script path (.ld file) to pass to the linker command",
		"minLength": 1
	})json"_ojson;

	{
		m_nonIndexedDefs[Defs::TargetSourceCxxLinkerOptions] = R"json({
			"type": "object",
			"additionalProperties": false,
			"description": "Addtional options (per compiler type) to add during the linking step."
		})json"_ojson;
		m_nonIndexedDefs[Defs::TargetSourceCxxLinkerOptions][kPatternProperties][kPatternCompilers] = R"json({
			"type": "string",
			"minLength": 1
		})json"_ojson;
	}

	defs[Defs::TargetSourceCxxLinks] = R"json({
		"type": "array",
		"uniqueItems": true,
		"minItems": 1,
		"description": "A list of dynamic links to use with the linker",
		"items": {
			"type": "string",
			"minLength": 1
		}
	})json"_ojson;
	defs[Defs::TargetSourceCxxLinks][kItems][kPattern] = kPatternTargetSourceLinks;

	defs[Defs::TargetSourceLocation] = R"json({
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
	defs[Defs::TargetSourceLocation][kOneOf][2][kPatternProperties][fmt::format("^exclude{}$", kPatternConditionConfigurationsPlatforms)] = defs[Defs::TargetSourceLocation][kOneOf][2][kProperties]["exclude"];
	defs[Defs::TargetSourceLocation][kOneOf][2][kPatternProperties][fmt::format("^include{}$", kPatternConditionConfigurationsPlatforms)] = defs[Defs::TargetSourceLocation][kOneOf][2][kProperties]["include"];

	defs[Defs::TargetSourceCxxMacOsFrameworkPaths] = R"json({
		"type": "array",
		"description": "A list of paths to search for MacOS Frameworks",
		"uniqueItems": true,
		"minItems": 1,
		"items": {
			"type": "string",
			"minLength": 1
		}
	})json"_ojson;

	defs[Defs::TargetSourceCxxMacOsFrameworks] = R"json({
		"type": "array",
		"description": "A list of MacOS Frameworks to link to the project",
		"uniqueItems": true,
		"minItems": 1,
		"items": {
			"type": "string",
			"minLength": 1
		}
	})json"_ojson;

	defs[Defs::TargetSourceCxxPrecompiledHeader] = R"json({
		"type": "string",
		"description": "Compile a header file as a pre-compiled header and include it in compilation of every object file in the project. Define a path relative to the workspace root.",
		"minLength": 1
	})json"_ojson;

	defs[Defs::TargetSourceCxxThreads] = R"json({
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

	defs[Defs::TargetSourceCxxCppModules] = R"json({
		"type": "boolean",
		"description": "true to enable C++20 modules, false to disable (default).",
		"default": false
	})json"_ojson;

	defs[Defs::TargetSourceCxxCppCoroutines] = R"json({
		"type": "boolean",
		"description": "true to enable C++20 coroutines, false to disable (default).",
		"default": false
	})json"_ojson;

	defs[Defs::TargetSourceCxxCppConcepts] = R"json({
		"type": "boolean",
		"description": "true to enable C++20 concepts in previous language standards (equivalent to '-fconcepts' or '-fconcepts-ts'), false to disable (default).",
		"default": false
	})json"_ojson;

	defs[Defs::TargetSourceCxxRunTimeTypeInfo] = R"json({
		"type": "boolean",
		"description": "true to include run-time type information (default), false to exclude.",
		"default": true
	})json"_ojson;

	defs[Defs::TargetSourceCxxExceptions] = R"json({
		"type": "boolean",
		"description": "true to use exceptions (default), false to turn off exceptions.",
		"default": true
	})json"_ojson;

	defs[Defs::TargetSourceCxxStaticLinking] = R"json({
		"description": "true to statically link against compiler libraries (libc++, etc.). false to dynamically link them.",
		"type": "boolean",
		"default": false
	})json"_ojson;

	defs[Defs::TargetSourceCxxStaticLinks] = R"json({
		"type": "array",
		"description": "A list of static links to use with the linker",
		"uniqueItems": true,
		"minItems": 1,
		"items": {
			"type": "string",
			"minLength": 1
		}
	})json"_ojson;
	defs[Defs::TargetSourceCxxStaticLinks][kItems][kPattern] = kPatternTargetSourceLinks;

	defs[Defs::TargetSourceCxxWarnings] = R"json({
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
	defs[Defs::TargetSourceCxxWarnings][kOneOf][1] = R"json({
		"type": "object",
		"additionalProperties": false,
		"description": "Warnings specific to each compiler"
	})json"_ojson;
	defs[Defs::TargetSourceCxxWarnings][kOneOf][1][kPatternProperties][kPatternCompilers] = defs[Defs::TargetSourceCxxWarnings][kOneOf][2];

	defs[Defs::TargetSourceCxxWarnings][kOneOf][2][kItems][kExamples] = {
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
	defs[Defs::TargetSourceCxxWarnings][kOneOf][1][kPatternProperties][kPatternCompilers][kItems][kExamples] = defs[Defs::TargetSourceCxxWarnings][kOneOf][2][kItems][kExamples];

	defs[Defs::TargetSourceCxxWindowsAppManifest] = R"json({
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

	defs[Defs::TargetSourceCxxWindowsAppIcon] = R"json({
		"type": "string",
		"description": "The windows icon to use for the project. Only applies to executable targets (kind=executable)",
		"minLength": 1
	})json"_ojson;

	/*defs[Defs::TargetSourceCxxWindowsOutputDef] = R"json({
		"type": "boolean",
		"description": "If true for a shared library (kind=sharedLibrary) target on Windows, a .def file will be created",
		"default": false
	})json"_ojson;*/

	defs[Defs::TargetSourceCxxWindowsSubSystem] = R"json({
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

	defs[Defs::TargetSourceCxxWindowsEntryPoint] = R"json({
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

	//

	defs[Defs::TargetScriptFile] = R"json({
		"description": "The relative path to a script file to run.",
		"type": "string",
		"minLength": 1
	})json"_ojson;

	//

	defs[Defs::TargetCMakeLocation] = R"json({
		"type": "string",
		"description": "The folder path of the root CMakeLists.txt for the project.",
		"minLength": 1
	})json"_ojson;

	defs[Defs::TargetCMakeBuildFile] = R"json({
		"type": "string",
		"description": "The build file to use, if not CMakeLists.txt, relative to the location. (-C)",
		"minLength": 1
	})json"_ojson;

	defs[Defs::TargetCMakeDefines] = R"json({
		"type": "array",
		"description": "Macro definitions to be passed into CMake. (-D)",
		"uniqueItems": true,
		"minItems": 1,
		"items": {
			"type": "string",
			"minLength": 1
		}
	})json"_ojson;

	defs[Defs::TargetCMakeRecheck] = R"json({
		"type": "boolean",
		"description": "If true, CMake will be invoked each time during the build.",
		"default": true
	})json"_ojson;

	defs[Defs::TargetCMakeRebuild] = R"json({
		"type": "boolean",
		"description": "If true, the CMake build folder will be cleaned and rebuilt when a rebuild is requested.",
		"default": true
	})json"_ojson;

	defs[Defs::TargetCMakeToolset] = R"json({
		"type": "string",
		"description": "A toolset to be passed to CMake with the -T option.",
		"minLength": 1
	})json"_ojson;

	//

	defs[Defs::TargetChaletLocation] = R"json({
		"type": "string",
		"description": "The folder path of the root chalet.json for the project.",
		"minLength": 1
	})json"_ojson;

	defs[Defs::TargetChaletBuildFile] = R"json({
		"type": "string",
		"description": "The build file to use, if not chalet.json, relative to the location.",
		"minLength": 1
	})json"_ojson;

	defs[Defs::TargetChaletRecheck] = R"json({
		"type": "boolean",
		"description": "If true, Chalet will be invoked each time during the build.",
		"default": true
	})json"_ojson;

	defs[Defs::TargetChaletRebuild] = R"json({
		"type": "boolean",
		"description": "If true, the Chalet build folder will be cleaned and rebuilt when a rebuild is requested.",
		"default": true
	})json"_ojson;

	defs[Defs::TargetCMakeRunExecutable] = R"json({
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
			"description": "Properties to describe a single build configuration type.",
			"additionalProperties": false
		})json"_ojson;
		configuration[kProperties]["debugSymbols"] = getDefinition(Defs::ConfigurationDebugSymbols);
		configuration[kProperties]["enableProfiling"] = getDefinition(Defs::ConfigurationEnableProfiling);
		configuration[kProperties]["linkTimeOptimization"] = getDefinition(Defs::ConfigurationLinkTimeOptimizations);
		configuration[kProperties]["optimizationLevel"] = getDefinition(Defs::ConfigurationOptimizationLevel);
		configuration[kProperties]["stripSymbols"] = getDefinition(Defs::ConfigurationStripSymbols);
		configuration[kProperties]["sanitize"] = getDefinition(Defs::ConfigurationSanitize);
		defs[Defs::Configuration] = std::move(configuration);
	}

	{
		auto distributionTarget = R"json({
			"type": "object",
			"description": "Properties to describe an individual bundle.",
			"additionalProperties": false,
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
		distributionTarget[kProperties] = Json::object();
		distributionTarget[kProperties]["buildTargets"] = getDefinition(Defs::DistributionBundleBuildTargets);
		distributionTarget[kProperties]["condition"] = getDefinition(Defs::DistributionCondition);
		distributionTarget[kProperties]["configuration"] = getDefinition(Defs::DistributionConfiguration);
		distributionTarget[kProperties]["description"] = getDefinition(Defs::TargetDescription);
		distributionTarget[kProperties]["exclude"] = getDefinition(Defs::DistributionBundleExclude);
		distributionTarget[kProperties]["include"] = getDefinition(Defs::DistributionBundleInclude);
		distributionTarget[kProperties]["includeDependentSharedLibraries"] = getDefinition(Defs::DistributionBundleIncludeDependentSharedLibraries);
		distributionTarget[kProperties]["kind"] = getDefinition(Defs::DistributionKind);
		distributionTarget[kProperties]["linuxDesktopEntry"] = m_nonIndexedDefs[Defs::DistributionBundleLinuxDesktopEntry];
		distributionTarget[kProperties]["macosBundle"] = m_nonIndexedDefs[Defs::DistributionBundleMacOSBundle];
		distributionTarget[kProperties]["mainExecutable"] = getDefinition(Defs::DistributionBundleMainExecutable);
		distributionTarget[kProperties]["subdirectory"] = getDefinition(Defs::DistributionBundleOutputDirectory);
		distributionTarget[kPatternProperties][fmt::format("^include{}$", kPatternConditionPlatforms)] = getDefinition(Defs::DistributionBundleInclude);
		distributionTarget[kPatternProperties][fmt::format("^exclude{}$", kPatternConditionPlatforms)] = getDefinition(Defs::DistributionBundleExclude);
		defs[Defs::DistributionBundle] = std::move(distributionTarget);
	}

	{
		auto distributionArchive = R"json({
			"type": "object",
			"description": "Properties to describe an individual distribution archive.",
			"additionalProperties": false,
			"required": [
				"kind",
				"include"
			]
		})json"_ojson;
		distributionArchive[kProperties]["condition"] = getDefinition(Defs::DistributionCondition);
		distributionArchive[kProperties]["description"] = getDefinition(Defs::TargetDescription);
		distributionArchive[kProperties]["include"] = getDefinition(Defs::DistributionArchiveInclude);
		distributionArchive[kProperties]["kind"] = getDefinition(Defs::DistributionKind);
		distributionArchive[kPatternProperties][fmt::format("^include{}$", kPatternConditionPlatforms)] = getDefinition(Defs::DistributionArchiveInclude);
		defs[Defs::DistributionArchive] = std::move(distributionArchive);
	}

	{
		auto distributionMacosDiskImage = R"json({
			"type": "object",
			"description": "Properties to describe a macos disk image (dmg). Implies 'condition: macos'",
			"additionalProperties": false,
			"required": [
				"kind",
				"size",
				"positions"
			]
		})json"_ojson;
		distributionMacosDiskImage[kProperties]["background"] = defs[Defs::DistributionMacosDiskImageBackground];
		distributionMacosDiskImage[kProperties]["description"] = getDefinition(Defs::TargetDescription);
		distributionMacosDiskImage[kProperties]["iconSize"] = getDefinition(Defs::DistributionMacosDiskImageIconSize);
		distributionMacosDiskImage[kProperties]["kind"] = getDefinition(Defs::DistributionKind);
		distributionMacosDiskImage[kProperties]["pathbarVisible"] = getDefinition(Defs::DistributionMacosDiskImagePathbarVisible);
		distributionMacosDiskImage[kProperties]["positions"] = m_nonIndexedDefs[Defs::DistributionMacosDiskImagePositions];
		distributionMacosDiskImage[kProperties]["size"] = m_nonIndexedDefs[Defs::DistributionMacosDiskImageSize];
		distributionMacosDiskImage[kProperties]["textSize"] = getDefinition(Defs::DistributionMacosDiskImageTextSize);
		defs[Defs::DistributionMacosDiskImage] = std::move(distributionMacosDiskImage);
	}

	{
		auto distributionWinNullsoft = R"json({
			"type": "object",
			"description": "Properties to describe an NSIS installer. Implies 'condition: windows'",
			"additionalProperties": false,
			"required": [
				"kind",
				"nsisScript"
			]
		})json"_ojson;
		distributionWinNullsoft[kProperties]["description"] = getDefinition(Defs::TargetDescription);
		distributionWinNullsoft[kProperties]["kind"] = getDefinition(Defs::DistributionKind);
		distributionWinNullsoft[kProperties]["nsisScript"] = getDefinition(Defs::DistributionWindowsNullsoftInstallerScript);
		distributionWinNullsoft[kProperties]["pluginDirs"] = getDefinition(Defs::DistributionWindowsNullsoftInstallerPluginDirs);
		distributionWinNullsoft[kProperties]["defines"] = getDefinition(Defs::DistributionWindowsNullsoftInstallerDefines);
		defs[Defs::DistributionWindowsNullsoftInstaller] = std::move(distributionWinNullsoft);
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
		externalDependency[kOneOf][1][kProperties]["branch"] = getDefinition(Defs::ExternalDependencyGitBranch);
		externalDependency[kOneOf][1][kProperties]["commit"] = getDefinition(Defs::ExternalDependencyGitCommit);
		externalDependency[kOneOf][1][kProperties]["repository"] = getDefinition(Defs::ExternalDependencyGitRepository);
		externalDependency[kOneOf][1][kProperties]["submodules"] = getDefinition(Defs::ExternalDependencyGitSubmodules);
		defs[Defs::ExternalDependency] = std::move(externalDependency);
	}

	{
		auto sourceTargetCxx = R"json({
			"type": "object",
			"additionalProperties": false
		})json"_ojson;
		sourceTargetCxx[kProperties]["cStandard"] = getDefinition(Defs::TargetSourceCxxCStandard);
		sourceTargetCxx[kProperties]["compileOptions"] = m_nonIndexedDefs[Defs::TargetSourceCxxCompileOptions];
		sourceTargetCxx[kProperties]["cppConcepts"] = getDefinition(Defs::TargetSourceCxxCppConcepts);
		sourceTargetCxx[kProperties]["cppCoroutines"] = getDefinition(Defs::TargetSourceCxxCppCoroutines);
		sourceTargetCxx[kProperties]["cppModules"] = getDefinition(Defs::TargetSourceCxxCppModules);
		sourceTargetCxx[kProperties]["cppStandard"] = getDefinition(Defs::TargetSourceCxxCppStandard);
		sourceTargetCxx[kProperties]["defines"] = getDefinitionwithCompilerOptions(Defs::TargetSourceCxxDefines);
		sourceTargetCxx[kProperties]["exceptions"] = getDefinition(Defs::TargetSourceCxxExceptions);
		sourceTargetCxx[kProperties]["includeDirs"] = getDefinitionwithCompilerOptions(Defs::TargetSourceCxxIncludeDirs);
		sourceTargetCxx[kProperties]["libDirs"] = getDefinitionwithCompilerOptions(Defs::TargetSourceCxxLibDirs);
		sourceTargetCxx[kProperties]["linkerScript"] = getDefinition(Defs::TargetSourceCxxLinkerScript);
		sourceTargetCxx[kProperties]["linkerOptions"] = m_nonIndexedDefs[Defs::TargetSourceCxxLinkerOptions];
		sourceTargetCxx[kProperties]["links"] = getDefinitionwithCompilerOptions(Defs::TargetSourceCxxLinks);
		sourceTargetCxx[kProperties]["macosFrameworkPaths"] = getDefinition(Defs::TargetSourceCxxMacOsFrameworkPaths);
		sourceTargetCxx[kProperties]["macosFrameworks"] = getDefinition(Defs::TargetSourceCxxMacOsFrameworks);
		sourceTargetCxx[kProperties]["pch"] = getDefinition(Defs::TargetSourceCxxPrecompiledHeader);
		sourceTargetCxx[kProperties]["threads"] = getDefinition(Defs::TargetSourceCxxThreads);
		sourceTargetCxx[kProperties]["rtti"] = getDefinition(Defs::TargetSourceCxxRunTimeTypeInfo);
		sourceTargetCxx[kProperties]["staticLinking"] = getDefinition(Defs::TargetSourceCxxStaticLinking);
		sourceTargetCxx[kProperties]["staticLinks"] = getDefinitionwithCompilerOptions(Defs::TargetSourceCxxStaticLinks);
		sourceTargetCxx[kProperties]["warnings"] = getDefinition(Defs::TargetSourceCxxWarnings);
		// sourceTargetCxx[kProperties]["windowsOutputDef"] = getDefinition(Defs::TargetSourceCxxWindowsOutputDef);
		sourceTargetCxx[kProperties]["windowsApplicationIcon"] = getDefinition(Defs::TargetSourceCxxWindowsAppIcon);
		sourceTargetCxx[kProperties]["windowsApplicationManifest"] = getDefinition(Defs::TargetSourceCxxWindowsAppManifest);
		sourceTargetCxx[kProperties]["windowsSubSystem"] = getDefinition(Defs::TargetSourceCxxWindowsSubSystem);
		sourceTargetCxx[kProperties]["windowsEntryPoint"] = getDefinition(Defs::TargetSourceCxxWindowsEntryPoint);

		sourceTargetCxx[kPatternProperties][fmt::format("^cStandard{}$", kPatternConditionPlatforms)] = getDefinition(Defs::TargetSourceCxxCStandard);
		sourceTargetCxx[kPatternProperties][fmt::format("^cppModules{}$", kPatternConditionPlatforms)] = getDefinition(Defs::TargetSourceCxxCppModules);
		sourceTargetCxx[kPatternProperties][fmt::format("^cppCoroutines{}$", kPatternConditionPlatforms)] = getDefinition(Defs::TargetSourceCxxCppCoroutines);
		sourceTargetCxx[kPatternProperties][fmt::format("^cppConcepts{}$", kPatternConditionPlatforms)] = getDefinition(Defs::TargetSourceCxxCppConcepts);
		sourceTargetCxx[kPatternProperties][fmt::format("^cppStandard{}$", kPatternConditionPlatforms)] = getDefinition(Defs::TargetSourceCxxCppStandard);
		sourceTargetCxx[kPatternProperties][fmt::format("^defines{}$", kPatternConditionConfigurationsPlatforms)] = getDefinition(Defs::TargetSourceCxxDefines);
		sourceTargetCxx[kPatternProperties][fmt::format("^includeDirs{}$", kPatternConditionPlatforms)] = getDefinition(Defs::TargetSourceCxxIncludeDirs);
		sourceTargetCxx[kPatternProperties][fmt::format("^libDirs{}$", kPatternConditionPlatforms)] = getDefinition(Defs::TargetSourceCxxLibDirs);
		sourceTargetCxx[kPatternProperties][fmt::format("^linkerScript{}$", kPatternConditionPlatforms)] = getDefinition(Defs::TargetSourceCxxLinkerScript);
		sourceTargetCxx[kPatternProperties][fmt::format("^links{}$", kPatternConditionConfigurationsPlatforms)] = getDefinition(Defs::TargetSourceCxxLinks);
		sourceTargetCxx[kPatternProperties][fmt::format("^staticLinks{}$", kPatternConditionConfigurationsPlatforms)] = getDefinition(Defs::TargetSourceCxxStaticLinks);
		sourceTargetCxx[kPatternProperties][fmt::format("^threads{}$", kPatternConditionConfigurationsPlatforms)] = getDefinition(Defs::TargetSourceCxxThreads);
		sourceTargetCxx[kPatternProperties][fmt::format("^rtti{}$", kPatternConditionConfigurationsPlatforms)] = getDefinition(Defs::TargetSourceCxxRunTimeTypeInfo);
		sourceTargetCxx[kPatternProperties][fmt::format("^exceptions{}$", kPatternConditionConfigurationsPlatforms)] = getDefinition(Defs::TargetSourceCxxExceptions);
		sourceTargetCxx[kPatternProperties][fmt::format("^staticLinking{}$", kPatternConditionConfigurationsPlatforms)] = getDefinition(Defs::TargetSourceCxxStaticLinking);

		sourceTargetCxx[kPatternProperties][fmt::format("^windowsApplicationIcon{}$", kPatternConditionConfigurations)] = getDefinition(Defs::TargetSourceCxxWindowsAppIcon);
		sourceTargetCxx[kPatternProperties][fmt::format("^windowsApplicationManifest{}$", kPatternConditionConfigurations)] = getDefinition(Defs::TargetSourceCxxWindowsAppManifest);
		sourceTargetCxx[kPatternProperties][fmt::format("^windowsSubSystem{}$", kPatternConditionConfigurations)] = getDefinition(Defs::TargetSourceCxxWindowsSubSystem);
		sourceTargetCxx[kPatternProperties][fmt::format("^windowsEntryPoint{}$", kPatternConditionConfigurations)] = getDefinition(Defs::TargetSourceCxxWindowsEntryPoint);

		defs[Defs::TargetSourceCxx] = std::move(sourceTargetCxx);
	}

	{
		auto abstractSource = R"json({
			"type": "object",
			"additionalProperties": false
		})json"_ojson;
		abstractSource[kProperties]["language"] = getDefinition(Defs::TargetSourceLanguage);
		abstractSource[kProperties]["settings:Cxx"] = getDefinition(Defs::TargetSourceCxx);
		abstractSource[kProperties]["settings"] = R"json({
			"type": "object",
			"description": "Settings for each language",
			"additionalProperties": false
		})json"_ojson;
		abstractSource[kProperties]["settings"][kProperties]["Cxx"] = getDefinition(Defs::TargetSourceCxx);
		abstractSource[kPatternProperties][fmt::format("^language{}$", kPatternConditionPlatforms)] = getDefinition(Defs::TargetDescription);
		defs[Defs::TargetAbstract] = std::move(abstractSource);
	}
	{
		auto targetSource = R"json({
			"type": "object",
			"additionalProperties": false,
			"required": [ "kind" ]
		})json"_ojson;
		targetSource[kProperties]["condition"] = getDefinition(Defs::TargetCondition);
		targetSource[kProperties]["description"] = getDefinition(Defs::TargetDescription);
		targetSource[kProperties]["extends"] = getDefinition(Defs::TargetSourceExtends);
		targetSource[kProperties]["files"] = getDefinition(Defs::TargetSourceFiles);
		targetSource[kProperties]["kind"] = getDefinition(Defs::TargetKind);
		targetSource[kProperties]["language"] = getDefinition(Defs::TargetSourceLanguage);
		targetSource[kProperties]["location"] = getDefinition(Defs::TargetSourceLocation);
		targetSource[kProperties]["settings"] = R"json({
			"type": "object",
			"description": "Settings for each language",
			"additionalProperties": false
		})json"_ojson;
		targetSource[kProperties]["settings"][kProperties]["Cxx"] = getDefinition(Defs::TargetSourceCxx);
		targetSource[kProperties]["settings:Cxx"] = getDefinition(Defs::TargetSourceCxx);
		targetSource[kPatternProperties][fmt::format("^language{}$", kPatternConditionPlatforms)] = getDefinition(Defs::TargetDescription);
		defs[Defs::TargetSourceLibrary] = std::move(targetSource);

		//
		defs[Defs::TargetSourceExecutable] = defs[Defs::TargetSourceLibrary];
		defs[Defs::TargetSourceExecutable][kProperties]["runArguments"] = getDefinition(Defs::TargetRunTargetArguments);
		defs[Defs::TargetSourceExecutable][kProperties]["runDependencies"] = getDefinition(Defs::TargetRunDependencies);
		defs[Defs::TargetSourceExecutable][kProperties]["runTarget"] = getDefinition(Defs::TargetRunTarget);
		defs[Defs::TargetSourceExecutable][kPatternProperties][fmt::format("^runDependencies{}$", kPatternConditionConfigurationsPlatforms)] = getDefinition(Defs::TargetRunDependencies);
		defs[Defs::TargetSourceExecutable][kPatternProperties][fmt::format("^runTarget{}$", kPatternConditionConfigurationsPlatforms)] = getDefinition(Defs::TargetRunTarget);
	}

	{
		auto targetBuildScript = R"json({
			"type": "object",
			"additionalProperties": false,
			"required": [
				"kind"
			]
		})json"_ojson;
		targetBuildScript[kProperties]["condition"] = getDefinition(Defs::TargetCondition);
		targetBuildScript[kProperties]["description"] = getDefinition(Defs::TargetDescription);
		targetBuildScript[kProperties]["kind"] = getDefinition(Defs::TargetKind);
		// targetBuildScript[kProperties]["runArguments"] = getDefinition(Defs::TargetRunTargetArguments);
		targetBuildScript[kProperties]["runTarget"] = getDefinition(Defs::TargetRunTarget);
		targetBuildScript[kProperties]["file"] = getDefinition(Defs::TargetScriptFile);
		targetBuildScript[kPatternProperties][fmt::format("^runTarget{}$", kPatternConditionConfigurationsPlatforms)] = getDefinition(Defs::TargetRunTarget);
		targetBuildScript[kPatternProperties][fmt::format("^file{}$", kPatternConditionConfigurationsPlatforms)] = getDefinition(Defs::TargetScriptFile);
		defs[Defs::TargetScript] = std::move(targetBuildScript);
	}

	{
		auto distributionScript = R"json({
			"type": "object",
			"additionalProperties": false
		})json"_ojson;
		distributionScript[kProperties]["condition"] = getDefinition(Defs::DistributionCondition);
		distributionScript[kProperties]["description"] = getDefinition(Defs::TargetDescription);
		distributionScript[kProperties]["kind"] = getDefinition(Defs::DistributionKind);
		distributionScript[kProperties]["file"] = getDefinition(Defs::TargetScriptFile);
		distributionScript[kPatternProperties][fmt::format("^file{}$", kPatternConditionPlatforms)] = getDefinition(Defs::TargetScriptFile);
		defs[Defs::DistributionScript] = std::move(distributionScript);
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
		targetCMake[kProperties]["buildFile"] = getDefinition(Defs::TargetCMakeBuildFile);
		targetCMake[kProperties]["condition"] = getDefinition(Defs::TargetCondition);
		targetCMake[kProperties]["defines"] = getDefinition(Defs::TargetCMakeDefines);
		targetCMake[kProperties]["description"] = getDefinition(Defs::TargetDescription);
		targetCMake[kProperties]["kind"] = getDefinition(Defs::TargetKind);
		targetCMake[kProperties]["location"] = getDefinition(Defs::TargetCMakeLocation);
		targetCMake[kProperties]["recheck"] = getDefinition(Defs::TargetCMakeRecheck);
		targetCMake[kProperties]["rebuild"] = getDefinition(Defs::TargetCMakeRebuild);
		targetCMake[kProperties]["runArguments"] = getDefinition(Defs::TargetRunTargetArguments);
		targetCMake[kProperties]["runExecutable"] = getDefinition(Defs::TargetCMakeRunExecutable);
		targetCMake[kProperties]["runTarget"] = getDefinition(Defs::TargetRunTarget);
		targetCMake[kProperties]["toolset"] = getDefinition(Defs::TargetCMakeToolset);
		targetCMake[kPatternProperties][fmt::format("^buildFile{}$", kPatternConditionConfigurationsPlatforms)] = getDefinition(Defs::TargetCMakeBuildFile);
		targetCMake[kPatternProperties][fmt::format("^defines{}$", kPatternConditionConfigurationsPlatforms)] = getDefinition(Defs::TargetCMakeDefines);
		targetCMake[kPatternProperties][fmt::format("^toolset{}$", kPatternConditionConfigurationsPlatforms)] = getDefinition(Defs::TargetCMakeToolset);
		targetCMake[kPatternProperties][fmt::format("^runTarget{}$", kPatternConditionConfigurationsPlatforms)] = getDefinition(Defs::TargetRunTarget);
		targetCMake[kPatternProperties][fmt::format("^runExecutable{}$", kPatternConditionConfigurationsPlatforms)] = getDefinition(Defs::TargetCMakeRunExecutable);
		defs[Defs::TargetCMake] = std::move(targetCMake);
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
		targetChalet[kProperties]["buildFile"] = getDefinition(Defs::TargetChaletBuildFile);
		targetChalet[kProperties]["condition"] = getDefinition(Defs::TargetCondition);
		targetChalet[kProperties]["description"] = getDefinition(Defs::TargetDescription);
		targetChalet[kProperties]["kind"] = getDefinition(Defs::TargetKind);
		targetChalet[kProperties]["location"] = getDefinition(Defs::TargetChaletLocation);
		targetChalet[kProperties]["recheck"] = getDefinition(Defs::TargetChaletRecheck);
		targetChalet[kProperties]["rebuild"] = getDefinition(Defs::TargetChaletRebuild);
		targetChalet[kPatternProperties][fmt::format("^buildFile{}$", kPatternConditionConfigurationsPlatforms)] = getDefinition(Defs::TargetChaletBuildFile);
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
		case Defs::ConfigurationDebugSymbols: return "configuration-debugSymbols";
		case Defs::ConfigurationEnableProfiling: return "configuration-enableProfiling";
		case Defs::ConfigurationLinkTimeOptimizations: return "configuration-linkTimeOptimizations";
		case Defs::ConfigurationOptimizationLevel: return "configuration-optimizationLevel";
		case Defs::ConfigurationStripSymbols: return "configuration-stripSymbols";
		case Defs::ConfigurationSanitize: return "configuration-sanitize";
		//
		case Defs::DistributionKind: return "dist-kind";
		case Defs::DistributionConfiguration: return "dist-configuration";
		case Defs::DistributionCondition: return "dist-condition";
		//
		case Defs::DistributionBundle: return "dist-bundle";
		case Defs::DistributionBundleInclude: return "dist-bundle-include";
		case Defs::DistributionBundleExclude: return "dist-bundle-exclude";
		case Defs::DistributionBundleMainExecutable: return "dist-bundle-mainExecutable";
		case Defs::DistributionBundleOutputDirectory: return "dist-bundle-subdirectory";
		case Defs::DistributionBundleBuildTargets: return "dist-bundle-buildTargets";
		case Defs::DistributionBundleIncludeDependentSharedLibraries: return "dist-bundle-includeDependentSharedLibraries";
		case Defs::DistributionBundleMacOSBundle: return "dist-bundle-macosBundle";
		case Defs::DistributionBundleLinuxDesktopEntry: return "dist-bundle-linuxDesktopEntry";
		//
		case Defs::DistributionScript: return "dist-script";
		//
		case Defs::DistributionArchive: return "dist-archive";
		case Defs::DistributionArchiveInclude: return "dist-archive-include";
		//
		case Defs::DistributionMacosDiskImage: return "dist-macos-disk-image";
		case Defs::DistributionMacosDiskImagePathbarVisible: return "dist-macos-disk-image-pathbarVisible";
		case Defs::DistributionMacosDiskImageIconSize: return "dist-macos-disk-image-iconSize";
		case Defs::DistributionMacosDiskImageTextSize: return "dist-macos-disk-image-textSize";
		case Defs::DistributionMacosDiskImageBackground: return "dist-macos-disk-image-background";
		case Defs::DistributionMacosDiskImageSize: return "dist-macos-disk-image-size";
		case Defs::DistributionMacosDiskImagePositions: return "dist-macos-disk-image-positions";
		//
		case Defs::DistributionWindowsNullsoftInstaller: return "dist-windows-nullsoft-installer";
		case Defs::DistributionWindowsNullsoftInstallerScript: return "dist-windows-nullsoft-installer-nsisScript";
		case Defs::DistributionWindowsNullsoftInstallerPluginDirs: return "dist-windows-nullsoft-installer-pluginDirs";
		case Defs::DistributionWindowsNullsoftInstallerDefines: return "dist-windows-nullsoft-installer-defines";
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
		case Defs::TargetSourceExtends: return "target-source-extends";
		case Defs::TargetSourceFiles: return "target-source-files";
		case Defs::TargetSourceLocation: return "target-source-location";
		case Defs::TargetSourceLanguage: return "target-source-language";
		//
		case Defs::TargetAbstract: return "target-abstract";
		case Defs::TargetSourceExecutable: return "target-source-executable";
		case Defs::TargetSourceLibrary: return "target-source-library";
		//
		case Defs::TargetSourceCxx: return "target-source-cxx";
		case Defs::TargetSourceCxxCStandard: return "target-source-cxx-cStandard";
		case Defs::TargetSourceCxxCppStandard: return "target-source-cxx-cppStandard";
		case Defs::TargetSourceCxxDefines: return "target-source-cxx-defines";
		case Defs::TargetSourceCxxIncludeDirs: return "target-source-cxx-includeDirs";
		case Defs::TargetSourceCxxLibDirs: return "target-source-cxx-libDirs";
		case Defs::TargetSourceCxxLinkerScript: return "target-source-cxx-linkerScript";
		case Defs::TargetSourceCxxLinks: return "target-source-cxx-links";
		case Defs::TargetSourceCxxMacOsFrameworkPaths: return "target-source-cxx-macosFrameworkPaths";
		case Defs::TargetSourceCxxMacOsFrameworks: return "target-source-cxx-macosFrameworks";
		case Defs::TargetSourceCxxPrecompiledHeader: return "target-source-cxx-pch";
		case Defs::TargetSourceCxxThreads: return "target-source-cxx-threads";
		case Defs::TargetSourceCxxCppModules: return "target-source-cxx-cppModules";
		case Defs::TargetSourceCxxCppCoroutines: return "target-source-cxx-cppCoroutines";
		case Defs::TargetSourceCxxCppConcepts: return "target-source-cxx-cppConcepts";
		case Defs::TargetSourceCxxRunTimeTypeInfo: return "target-source-cxx-rtti";
		case Defs::TargetSourceCxxExceptions: return "target-source-cxx-exceptions";
		case Defs::TargetSourceCxxStaticLinking: return "target-source-cxx-staticLinking";
		case Defs::TargetSourceCxxStaticLinks: return "target-source-cxx-staticLinks";
		case Defs::TargetSourceCxxWarnings: return "target-source-cxx-warnings";
		case Defs::TargetSourceCxxWindowsAppManifest: return "target-source-cxx-windowsApplicationManifest";
		case Defs::TargetSourceCxxWindowsAppIcon: return "target-source-cxx-windowsAppIcon";
		// case Defs::TargetSourceCxxWindowsOutputDef: return "target-source-cxx-windowsOutputDef";
		case Defs::TargetSourceCxxWindowsSubSystem: return "target-source-cxx-windowsSubSystem";
		case Defs::TargetSourceCxxWindowsEntryPoint: return "target-source-cxx-windowsEntryPoint";
		//
		case Defs::TargetScript: return "target-script";
		case Defs::TargetScriptFile: return "target-script-file";
		//
		case Defs::TargetCMake: return "target-cmake";
		case Defs::TargetCMakeLocation: return "target-cmake-location";
		case Defs::TargetCMakeBuildFile: return "target-cmake-buildFile";
		case Defs::TargetCMakeDefines: return "target-cmake-defines";
		case Defs::TargetCMakeRecheck: return "target-cmake-recheck";
		case Defs::TargetCMakeRebuild: return "target-cmake-rebuild";
		case Defs::TargetCMakeToolset: return "target-cmake-toolset";
		case Defs::TargetCMakeRunExecutable: return "target-cmake-runExecutable";
		//
		case Defs::TargetChalet: return "target-chalet";
		case Defs::TargetChaletLocation: return "target-chalet-location";
		case Defs::TargetChaletBuildFile: return "target-chalet-buildFile";
		case Defs::TargetChaletRecheck: return "target-chalet-recheck";
		case Defs::TargetChaletRebuild: return "target-chalet-rebuild";

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

	ret[kPatternProperties][fmt::format("^abstracts:(\\*|{})$", kPatternAbstractName)] = getDefinition(Defs::TargetAbstract);
	ret[kPatternProperties][fmt::format("^abstracts:(\\*|{})$", kPatternAbstractName)][kDescription] = "An abstract build target. 'abstracts:*' is a special target that gets implicitely added to each project";

	ret[kProperties]["abstracts"] = R"json({
		"type": "object",
		"additionalProperties": false,
		"description": "A list of abstract build targets"
	})json"_ojson;
	ret[kProperties]["abstracts"][kPatternProperties][fmt::format("^(\\*|{})$", kPatternAbstractName)] = getDefinition(Defs::TargetAbstract);
	ret[kProperties]["abstracts"][kPatternProperties][fmt::format("^(\\*|{})$", kPatternAbstractName)][kDescription] = "An abstract build target. '*' is a special target that gets implicitely added to each project.";

	ret[kProperties]["defaultConfigurations"] = R"json({
		"type": "array",
		"description": "An array of allowed default build configuration names.",
		"uniqueItems": true,
		"default": [],
		"items": {
			"type": "string",
			"description": "A default configuration name",
			"minLength": 1
		}
	})json"_ojson;
	ret[kProperties]["defaultConfigurations"][kDefault] = BuildConfiguration::getDefaultBuildConfigurationNames();
	ret[kProperties]["defaultConfigurations"][kItems][kEnum] = BuildConfiguration::getDefaultBuildConfigurationNames();

	ret[kProperties]["configurations"] = R"json({
		"type": "object",
		"description": "An object of custom build configurations. If one has the same name as a default build configuration, the default will be replaced.",
		"additionalProperties": false
	})json"_ojson;
	ret[kProperties]["configurations"][kPatternProperties][R"(^[A-Za-z]{3,}$)"] = getDefinition(Defs::Configuration);

	ret[kProperties]["distribution"] = R"json({
		"type": "object",
		"additionalProperties": false,
		"description": "A sequential list of distribution targets to be created during the bundle phase."
	})json"_ojson;
	ret[kProperties]["distribution"][kPatternProperties][kPatternDistributionName] = R"json({
		"type": "object",
		"description": "A single distribution target.",
		"if": { "properties": {
			"kind": { "const": "bundle" }
		}},
		"then": {},
		"else": {
			"if": { "properties": {
				"kind": { "const": "script" }
			}},
			"then": {},
			"else": {
				"if": { "properties": {
					"kind": { "const": "archive" }
				}},
				"then": {},
				"else": {
					"if": { "properties": {
						"kind": { "const": "macosDiskImage" }
					}},
					"then": {},
					"else": {
						"if": { "properties": {
							"kind": { "const": "windowsNullsoftInstaller" }
						}},
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
	//
	ret[kProperties]["distribution"][kPatternProperties][kPatternDistributionName][kThen] = getDefinition(Defs::DistributionBundle);
	ret[kProperties]["distribution"][kPatternProperties][kPatternDistributionName][kElse][kThen] = getDefinition(Defs::DistributionScript);
	ret[kProperties]["distribution"][kPatternProperties][kPatternDistributionName][kElse][kElse][kThen] = getDefinition(Defs::DistributionArchive);
	ret[kProperties]["distribution"][kPatternProperties][kPatternDistributionName][kElse][kElse][kElse][kThen] = getDefinition(Defs::DistributionMacosDiskImage);
	ret[kProperties]["distribution"][kPatternProperties][kPatternDistributionName][kElse][kElse][kElse][kElse][kThen] = getDefinition(Defs::DistributionWindowsNullsoftInstaller);

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
		"if": { "properties": {
			"kind": { "const": "executable" }
		}},
		"then": {},
		"else": {
			"if": { "properties": {
				"kind": { "enum": [ "staticLibrary", "sharedLibrary" ] }
			}},
			"then": {},
			"else": {
				"if": { "properties": {
					"kind": { "const": "cmakeProject" }
				}},
				"then": {},
				"else": {
					"if": { "properties": {
						"kind": { "const": "chaletProject" }
					}},
					"then": {},
					"else": {
						"if": { "properties": {
							"kind": { "const": "script" }
						}},
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
	ret[kProperties][targets][kPatternProperties][kPatternTargetName][kThen] = getDefinition(Defs::TargetSourceExecutable);
	ret[kProperties][targets][kPatternProperties][kPatternTargetName][kElse][kThen] = getDefinition(Defs::TargetSourceLibrary);
	ret[kProperties][targets][kPatternProperties][kPatternTargetName][kElse][kElse][kThen] = getDefinition(Defs::TargetCMake);
	ret[kProperties][targets][kPatternProperties][kPatternTargetName][kElse][kElse][kElse][kThen] = getDefinition(Defs::TargetChalet);
	ret[kProperties][targets][kPatternProperties][kPatternTargetName][kElse][kElse][kElse][kElse][kThen] = getDefinition(Defs::TargetScript);
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
