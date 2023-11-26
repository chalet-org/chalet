/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "ChaletJson/ChaletJsonSchema.hpp"

#include "FileTemplates/PlatformFileTemplates.hpp"

#include "Core/CommandLineInputs.hpp"
#include "State/BuildConfiguration.hpp"
#include "System/SuppressIntellisense.hpp"
#include "Utility/String.hpp"
#include "Json/JsonComments.hpp"
#include "Json/JsonKeys.hpp"

namespace chalet
{
/*****************************************************************************/
ChaletJsonSchema::ChaletJsonSchema(const CommandLineInputs& inInputs) :
	m_inputs(inInputs),
	kPatternTargetName(R"regex(^[\w\-+.]{3,}$)regex"),
	kPatternAbstractName(R"regex([A-Za-z\-_]+)regex"),
	kPatternTargetSourceLinks(R"regex(^[\w\-+./\{\}\$:]+$)regex"),
	kPatternDistributionName(R"regex(^(([\w\-+. ()]+)|(\$\{(targetTriple|toolchainName|configuration|architecture|buildDir)\}))+$)regex"),
	kPatternDistributionNameSimple(R"regex(^[\w\-+. ()]{2,}$)regex"),
	kPatternConditions(R"regex(\[(\w*:(!?[\w\-]+|\{!?[\w\-]+(,!?[\w\-]+)*\}))([\+\|](\w*:(!?[\w\-]+|\{!?[\w\-]+(,!?[\w\-]+)*\})))*\])regex") // https://regexr.com/6jni8
{
}

/*****************************************************************************/
Json ChaletJsonSchema::get(const CommandLineInputs& inInputs)
{
	ChaletJsonSchema schema(inInputs);
	return schema.get();
}

/*****************************************************************************/
ChaletJsonSchema::DefinitionMap ChaletJsonSchema::getDefinitions()
{
	DefinitionMap defs;

	//
	// workspace metadata / root
	//
	defs[Defs::WorkspaceName] = R"json({
		"type": "string",
		"description": "Metadata: A name to describe the entire workspace.",
		"minLength": 1,
		"pattern": "^[\\w\\-+ ]+$"
	})json"_ojson;

	defs[Defs::WorkspaceVersion] = R"json({
		"type": "string",
		"description": "Metadata: A version to give to the entire workspace.",
		"minLength": 1
	})json"_ojson;
	defs[Defs::WorkspaceVersion][SKeys::Pattern] = R"regex(^((\d+\.){1,3})?\d+$)regex";

	defs[Defs::WorkspaceDescription] = R"json({
		"type": "string",
		"description": "Metadata: A description for the workspace.",
		"minLength": 1
	})json"_ojson;

	defs[Defs::WorkspaceHomepage] = R"json({
		"type": "string",
		"description": "Metadata: A homepage URL for the workspace.",
		"minLength": 1
	})json"_ojson;

	defs[Defs::WorkspaceAuthor] = R"json({
		"type": "string",
		"description": "Metadata: An individual or business entity involved in creating or maintaining the workspace.",
		"minLength": 1
	})json"_ojson;

	defs[Defs::WorkspaceLicense] = R"json({
		"type": "string",
		"description": "Metadata: A license identifier or text file path that describes how people are permitted or restricted to use this workspace.",
		"minLength": 1
	})json"_ojson;

	defs[Defs::WorkspaceReadme] = R"json({
		"type": "string",
		"description": "Metadata: A path to the readme file of the workspace.",
		"minLength": 1
	})json"_ojson;

	//
	// configurations
	//
	defs[Defs::ConfigurationDebugSymbols] = R"json({
		"type": "boolean",
		"description": "",
		"default": false
	})json"_ojson;
	defs[Defs::ConfigurationDebugSymbols]["description"] = fmt::format("true to include debug symbols, false otherwise.\nIn GNU-based compilers, this is equivalent to the `-g3` option (`-g` & macro information expansion) and forces `-O0` if the optimizationLevel is not `0` or `debug`.\nIn MSVC, this enables `/debug`, `/incremental` and forces `/Od` if the optimizationLevel is not `0` or `debug`.\nAdditionally, `_DEBUG` will be defined in `*-pc-windows-msvc` targets.\nThis flag is also the determining factor whether the `:debug` suffix is used in a {} property.", m_inputs.defaultInputFile());

	defs[Defs::ConfigurationEnableProfiling] = R"json({
		"type": "boolean",
		"description": "true to enable profiling for this configuration, false otherwise.\nIn GNU-based compilers, this is equivalent to the `-pg` option\nIn MSVC, this adds the `/debug:FULL` and `/profile` options.\nIf profiling is enabled and the project is run, a compatible profiler application will be launched when the program is run.",
		"default": false
	})json"_ojson;

	defs[Defs::ConfigurationInterproceduralOptimization] = R"json({
		"type": "boolean",
		"description": "true to use interprocedural optimizations, false otherwise.\nIn GCC, this enables link-time optimizations - the equivalent to passing the `-flto` & `-fno-fat-lto-objects` options to the compiler, and `-flto` to the linker.\nIn MSVC, this performs whole program optimizations - the equivalent to passing `/GL` to cl.exe and `/LTCG` to link.exe and lib.exe\nIn Clang, so far, this does nothing.",
		"default": false
	})json"_ojson;

	defs[Defs::ConfigurationOptimizationLevel] = R"json({
		"type": "string",
		"description": "The optimization level of the build.\nIn GNU-based compilers, This maps 1:1 with its respective `-O` option, except for debug - `-Od` and size - `-Os`.\nIn MSVC, it's mapped as follows: 0 - `/Od`, 1 - `/O1`, 2 - `/O2`, 3 - `/Ox`, size - `/Os`, fast - `/Ot`, debug - `/Od`\nIf this value is unset, no optimization level will be used (implying the compiler's default).",
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

	defs[Defs::ConfigurationSanitize] = makeArrayOrString(R"json({
		"type": "string",
		"description": "An array of sanitizers to enable. If combined with staticRuntimeLibrary, the selected sanitizers will be statically linked, if available by the toolchain.",
		"minLength": 1,
		"enum": [
			"address",
			"hwaddress",
			"thread",
			"memory",
			"leak",
			"undefined"
		]
	})json"_ojson);
	defs[Defs::ConfigurationSanitize][SKeys::OneOf][2] = R"json({
		"type": "boolean",
		"const": false
	})json"_ojson;
	defs[Defs::ConfigurationSanitize][SKeys::Default] = false;

	//
	// distribution
	// bundle, script, process, archive, macosDiskImage
	//
	defs[Defs::DistributionKind] = R"json({
		"type": "string",
		"description": "Whether the distribution target is a bundle, script, archive, or something platform-specific.",
		"minLength": 1
	})json"_ojson;

	defs[Defs::DistributionBundleInclude] = makeArrayOrString(R"json({
		"type": "string",
		"description": "A list of files or folders to copy into the output directory of the bundle.\nIn MacOS, these will be placed into the `Resources` folder of the application bundle.",
		"minLength": 1
	})json"_ojson);

	defs[Defs::DistributionBundleExclude] = makeArrayOrString(R"json({
		"type": "string",
		"description": "In folder paths that are included with `include`, exclude certain files or paths.\nCan accept a glob pattern.",
		"minLength": 1
	})json"_ojson);

	defs[Defs::DistributionBundleIncludeDependentSharedLibraries] = R"json({
		"type": "boolean",
		"description": "If true (default), any shared libraries that the bundle depends on will also be copied.",
		"default": true
	})json"_ojson;

	{
		auto includeRuntimeDlls = R"json({
			"type": "boolean",
			"description": "If true, include the Windows UCRT dlls if 'staticRuntimeLibrary' is set to false by the build target. false to exclude them (default). This only applies if 'includeDependentSharedLibraries' is set to true",
			"default": false
		})json"_ojson;

		m_nonIndexedDefs[Defs::DistributionBundleWindows] = R"json({
			"type": "object",
			"description": "Properties applicable to Windows application distribution.",
			"additionalProperties": false,
			"properties": {}
		})json"_ojson;
		m_nonIndexedDefs[Defs::DistributionBundleWindows][SKeys::Properties]["includeRuntimeDlls"] = std::move(includeRuntimeDlls);
	}

	{
		auto macosBundleType = R"json({
			"type": "string",
			"description": "The MacOS bundle type (only `.app` is supported currently)",
			"minLength": 1,
			"enum": [
				"app"
			],
			"default": "app"
		})json"_ojson;

		auto macosBundleIcon = R"json({
			"type": "string",
			"description": "The path to a MacOS bundle icon either in PNG or ICNS format (PNG 1024x1024 is recommended).\nIf the file is a .png, it will get converted to .icns during the bundle process.",
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
		macosInfoPropertyList[SKeys::OneOf][1]["default"] = JsonComments::parseLiteral(PlatformFileTemplates::macosInfoPlist());

		auto macosEntitlementsPropertyList = R"json({
			"description": "The path to a property list (.xml) file, .json file, or the properties themselves to describe the entitlements required to run the app. Only applies to codesigned bundles",
			"oneOf": [
				{
					"type": "string",
					"minLength": 1,
					"default": "Entitlements.plist.json"
				},
				{
					"type": "object"
				}
			]
		})json"_ojson;

		m_nonIndexedDefs[Defs::DistributionBundleMacOSBundle] = R"json({
			"type": "object",
			"description": "Properties to describe the MacOS bundle.",
			"additionalProperties": false,
			"required": [
				"type"
			],
			"properties": {}
		})json"_ojson;
		m_nonIndexedDefs[Defs::DistributionBundleMacOSBundle][SKeys::Properties]["entitlementsPropertyList"] = std::move(macosEntitlementsPropertyList);
		m_nonIndexedDefs[Defs::DistributionBundleMacOSBundle][SKeys::Properties]["icon"] = std::move(macosBundleIcon);
		m_nonIndexedDefs[Defs::DistributionBundleMacOSBundle][SKeys::Properties]["infoPropertyList"] = std::move(macosInfoPropertyList);
		m_nonIndexedDefs[Defs::DistributionBundleMacOSBundle][SKeys::Properties]["type"] = std::move(macosBundleType);
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
		m_nonIndexedDefs[Defs::DistributionBundleLinuxDesktopEntry][SKeys::Properties]["template"] = std::move(linuxDesktopEntryTemplate);
		m_nonIndexedDefs[Defs::DistributionBundleLinuxDesktopEntry][SKeys::Properties]["icon"] = std::move(linuxDesktopEntryIcon);
	}

	defs[Defs::DistributionBundleMainExecutable] = R"json({
		"type": "string",
		"description": "The name of the main executable project target.\nIf this property is not defined, the first executable in the `buildTargets` array of the bundle will be chosen as the main executable.",
		"minLength": 1
	})json"_ojson;

	defs[Defs::DistributionBundleSubDirectory] = R"json({
		"type": "string",
		"description": "The sub-directory to be placed inside of the `dist` directory (it not otherwise changed) to place this bundle along with all of its included resources and shared libraries.",
		"minLength": 1
	})json"_ojson;

	defs[Defs::DistributionBundleBuildTargets] = R"json({
		"description": "Either an array of build target names to include in this bundle. A single string value of `*` will include all build targets.\nIf `mainExecutable` is not defined, the first executable target in this list will be chosen as the main executable.",
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
	defs[Defs::DistributionBundleBuildTargets][SKeys::Items][SKeys::Pattern] = kPatternTargetName;

	//
	defs[Defs::DistributionArchiveFormat] = R"json({
		"type": "string",
		"description": "The archive format to use. If not specified, `zip` will be used.",
		"minLength": 1,
		"enum": [
			"zip",
			"tar"
		],
		"default": "zip"
	})json"_ojson;

	defs[Defs::DistributionArchiveInclude] = makeArrayOrString(R"json({
		"type": "string",
		"description": "A list of files or folders to add to the archive, relative to the root distribution directory. Glob patterns are also accepted. A single string value of `*` will archive everything in the bundle directory.",
		"minLength": 1
	})json"_ojson);
	defs[Defs::DistributionArchiveInclude][SKeys::OneOf][0][SKeys::Default] = "*";

	defs[Defs::DistributionArchiveMacosNotarizationProfile] = R"json({
		"type": "string",
		"description": "The keychain profile to use for notarization on macos. Requires Xcode 13 or higher",
		"minLength": 1
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
		"description": "Either a single path to a TIFF or a PNG background image, or paths to 1x/2x PNG background images.",
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
		"description": "The visible window dimensions of the disk image.",
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
		"description": "Icon positions for the root disk image paths.\nSpecifying the name of a bundle will include it in the image. Specifying `Applications` will include a symbolic link to the `/Applications` path.\nAdditionally, if there is a bundle named `Applications`, it will be ignored, and an error will be displayed.",
		"additionalProperties": false
	})json"_ojson;
	m_nonIndexedDefs[Defs::DistributionMacosDiskImagePositions][SKeys::PatternProperties][kPatternDistributionNameSimple] = R"json({
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
				"description": "The y position of the path's icon",
				"default": 80,
				"minimum": -1024,
				"maximum": 32000
			}
		}
	})json"_ojson;

	//
	// externalDependency
	// git, local, script
	//
	defs[Defs::ExternalDependencyKind] = R"json({
		"type": "string",
		"description": "Whether the external dependency is a git repository, local folder, or script.",
		"minLength": 1
	})json"_ojson;

	defs[Defs::ExternalDependencyGitRepository] = R"json({
		"type": "string",
		"description": "The url of the git repository.",
		"minLength": 1
	})json"_ojson;
	defs[Defs::ExternalDependencyGitRepository][SKeys::Pattern] = R"regex(^(?:git|ssh|git\+ssh|https?|git@[\w\-.]+):(\/\/)?(.*?)(\.git)?(\/?|#[\w\d\-._]+?)$)regex";

	defs[Defs::ExternalDependencyGitBranch] = R"json({
		"type": "string",
		"description": "The git branch to checkout. Uses the repository's default if not set.",
		"minLength": 1
	})json"_ojson;

	defs[Defs::ExternalDependencyGitCommit] = R"json({
		"type": "string",
		"description": "The SHA1 hash of the git commit to checkout.",
		"pattern": "^[0-9a-f]{7,40}$",
		"minLength": 1
	})json"_ojson;

	defs[Defs::ExternalDependencyGitTag] = R"json({
		"type": "string",
		"description": "The tag to checkout on the selected git branch. If it's blank or not found, the head of the branch will be checked out.",
		"minLength": 1
	})json"_ojson;

	defs[Defs::ExternalDependencyGitSubmodules] = R"json({
		"type": "boolean",
		"description": "Do submodules need to be cloned?",
		"default": false
	})json"_ojson;

	defs[Defs::ExternalDependencyLocalPath] = R"json({
		"type": "string",
		"description": "The local path to a dependency to build from. Can take `env` and `var` substitution variables. (ie. `${env:SOME_PATH}`)",
		"minLength": 1
	})json"_ojson;

	//
	// environment
	//
	defs[Defs::EnvironmentVariableValue] = R"json({
		"type": "string",
		"description": "The value to assign to an environment variable",
		"minLength": 1
	})json"_ojson;

	defs[Defs::EnvironmentSearchPaths] = makeArrayOrString(R"json({
		"type": "string",
		"description": "Any additional search paths to include. Accepts Chalet variables such as ${buildDir} & ${external:(name)}",
		"minLength": 1
	})json"_ojson);

	//
	// target
	//
	defs[Defs::TargetOutputDescription] = R"json({
		"type": "string",
		"description": "A description of the target to display in the build output.",
		"minLength": 1
	})json"_ojson;

	defs[Defs::TargetCondition] = R"json({
		"type": "string",
		"description": "A rule describing when to include this target in the build.",
		"minLength": 1
	})json"_ojson;
	defs[Defs::TargetCondition][SKeys::Pattern] = fmt::format("^{}$", kPatternConditions);

	defs[Defs::DistributionCondition] = R"json({
		"type": "string",
		"description": "A rule describing when to include this target in the distribution.",
		"minLength": 1
	})json"_ojson;
	defs[Defs::DistributionCondition][SKeys::Pattern] = fmt::format("^{}$", kPatternConditions);

	defs[Defs::ExternalDependencyCondition] = R"json({
		"type": "string",
		"description": "A rule describing when to include this dependency during the build. Only accepts env and platform variables.",
		"minLength": 1
	})json"_ojson;
	defs[Defs::ExternalDependencyCondition][SKeys::Pattern] = fmt::format("^{}$", kPatternConditions);

	defs[Defs::TargetSourceExtends] = R"json({
		"type": "string",
		"description": "An abstract source target template to extend from. Defaults to `*` implicitly.\n If `abstracts:*` is not defined, then effectively, nothing is extended.",
		"pattern": "",
		"minLength": 1,
		"default": "*"
	})json"_ojson;
	defs[Defs::TargetSourceExtends][SKeys::Pattern] = fmt::format("^(\\*|{})$", kPatternAbstractName);

	/*defs[Defs::TargetSourceFiles] = makeArrayOrString(R"json({
		"type": "string",
		"description": "Explicitly define the source files, relative to the working directory.",
		"minLength": 1
	})json"_ojson);*/
	defs[Defs::TargetSourceFiles] = R"json({
		"description": "Define the source files, relative to the working directory.",
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
	defs[Defs::TargetSourceFiles][SKeys::OneOf][2][SKeys::PatternProperties][fmt::format("^exclude{}$", kPatternConditions)] = defs[Defs::TargetSourceFiles][SKeys::OneOf][2][SKeys::Properties]["exclude"];
	defs[Defs::TargetSourceFiles][SKeys::OneOf][2][SKeys::PatternProperties][fmt::format("^include{}$", kPatternConditions)] = defs[Defs::TargetSourceFiles][SKeys::OneOf][2][SKeys::Properties]["include"];

	// staticLibrary, sharedLibrary, executable, cmakeProject, chaletProject, script, process
	//
	defs[Defs::TargetKind] = R"json({
		"type": "string",
		"description": "The type of the target's compiled binary, a script or external project.",
		"minLength": 1
	})json"_ojson;

	defs[Defs::TargetSourceLanguage] = R"json({
		"type": "string",
		"description": "The desired programming language of the project.",
		"minLength": 1,
		"enum": [
			"C",
			"C++",
			"Objective-C",
			"Objective-C++"
		],
		"default": "C++"
	})json"_ojson;

	defs[Defs::TargetSourceConfigureFiles] = makeArrayOrString(R"json({
		"type": "string",
		"description": "A list of files to copy into an intermediate build folder, which may include susbstitution variables formatted as either `@VAR` or `${VAR}`. These refer to metadata values at the workspace level or at the source target (project) level. They will be replaced with the variables' value, or an empty string if not recognized. Some backwards compatibility with CMake configure files are supported for convenience.\nThe variables are:\n`WORKSPACE_NAME` `WORKSPACE_DESCRIPTION` `WORKSPACE_AUTHOR` `WORKSPACE_HOMEPAGE_URL` `WORKSPACE_LICENSE` `WORKSPACE_README` `WORKSPACE_VERSION` `WORKSPACE_VERSION_MAJOR` `WORKSPACE_VERSION_MINOR` `WORKSPACE_VERSION_PATCH` `WORKSPACE_VERSION_TWEAK`\n`PROJECT_NAME` `PROJECT_DESCRIPTION` `PROJECT_AUTHOR` `PROJECT_HOMEPAGE_URL` `PROJECT_LICENSE` `PROJECT_README` `PROJECT_VERSION` `PROJECT_VERSION_MAJOR` `PROJECT_VERSION_MINOR` `PROJECT_VERSION_PATCH` `PROJECT_VERSION_TWEAK`\n`CMAKE_PROJECT_NAME` `CMAKE_PROJECT_DESCRIPTION` `CMAKE_PROJECT_AUTHOR` `CMAKE_PROJECT_HOMEPAGE_URL` `CMAKE_PROJECT_LICENSE` `CMAKE_PROJECT_README` `CMAKE_PROJECT_VERSION` `CMAKE_PROJECT_VERSION_MAJOR` `CMAKE_PROJECT_VERSION_MINOR` `CMAKE_PROJECT_VERSION_PATCH` `CMAKE_PROJECT_VERSION_TWEAK`\n\n`CMAKE_PROJECT_` variables are equivalent to `WORKSPACE_` so using them is a matter of compatibility and preference.",
		"minLength": 1
	})json"_ojson);

	//
	// workspace metadata / root
	//
	defs[Defs::TargetSourceMetadataName] = R"json({
		"type": "string",
		"description": "A name to describe the build target.",
		"minLength": 1,
		"pattern": "^[\\w\\-+ \\.\\$\\{\\}:]+$"
	})json"_ojson;

	defs[Defs::TargetSourceMetadataVersion] = R"json({
		"type": "string",
		"description": "A version to give to the build target.",
		"minLength": 1
	})json"_ojson;
	defs[Defs::TargetSourceMetadataVersion][SKeys::Pattern] = R"regex(^[\w\-+ \.\$\{\}:]+$)regex";

	defs[Defs::TargetSourceMetadataDescription] = R"json({
		"type": "string",
		"description": "A description for the build target.",
		"minLength": 1
	})json"_ojson;

	defs[Defs::TargetSourceMetadataHomepage] = R"json({
		"type": "string",
		"description": "A homepage URL for the build target.",
		"minLength": 1
	})json"_ojson;

	defs[Defs::TargetSourceMetadataAuthor] = R"json({
		"type": "string",
		"description": "An individual or business entity involved in creating or maintaining the build target.",
		"minLength": 1
	})json"_ojson;

	defs[Defs::TargetSourceMetadataLicense] = R"json({
		"type": "string",
		"description": "A license identifier or text file path that describes how people are permitted or restricted to use this build target.",
		"minLength": 1
	})json"_ojson;

	defs[Defs::TargetSourceMetadataReadme] = R"json({
		"type": "string",
		"description": "A path to the readme file of the build target.",
		"minLength": 1
	})json"_ojson;

	defs[Defs::TargetSourceCopyFilesOnRun] = makeArrayOrString(R"json({
		"type": "string",
		"description": "If this is the run target, a list of files that should be copied into the build folder before running. This is primarily meant for libraries that need to be resolved from the same directory as the run target. In the case of MacOS bundles, these will be copied inside the `MacOS` folder path alongside the executable.",
		"minLength": 1
	})json"_ojson);

	defs[Defs::TargetDefaultRunArguments] = makeArrayOrString(R"json({
		"type": "string",
		"description": "If this is the run target, a string of arguments to pass to the run command.",
		"minLength": 1
	})json"_ojson,
		false);

	defs[Defs::TargetSourceCxxCStandard] = R"json({
		"type": "string",
		"description": "The C standard to use in the compilation",
		"pattern": "^((c|gnu)\\d[\\dx]|(iso9899:(1990|199409|1999|199x|20\\d{2})))$",
		"minLength": 1,
		"default": "c11"
	})json"_ojson;

	defs[Defs::TargetSourceCxxCompileOptions] = makeArrayOrString(R"json({
		"type": "string",
		"description": "Addtional options per compiler type (via property conditions) to add during the compilation step.",
		"minLength": 1
	})json"_ojson);

	defs[Defs::TargetSourceCxxCppStandard] = R"json({
		"type": "string",
		"description": "The C++ standard to use during compilation",
		"pattern": "^(c|gnu)\\+\\+\\d[\\dxyzabc]$",
		"minLength": 1,
		"default": "c++17"
	})json"_ojson;

	defs[Defs::TargetSourceCxxDefines] = makeArrayOrString(R"json({
		"type": "string",
		"description": "Macro definitions to be used by the preprocessor",
		"minLength": 1
	})json"_ojson);

	defs[Defs::TargetSourceCxxIncludeDirs] = makeArrayOrString(R"json({
		"type": "string",
		"description": "A list of directories to include during compilation.",
		"minLength": 1
	})json"_ojson);

	defs[Defs::TargetSourceCxxLibDirs] = makeArrayOrString(R"json({
		"type": "string",
		"description": "Fallback search paths to look for static or dynamic libraries (`/usr/lib` is included by default)",
		"minLength": 1
	})json"_ojson);

	defs[Defs::TargetSourceCxxLinkerOptions] = makeArrayOrString(R"json({
		"type": "string",
		"description": "Addtional options per compiler type (via property conditions) to add during the linking step.",
		"minLength": 1
	})json"_ojson);

	{
		Json links = R"json({
			"type": "string",
			"description": "A list of dynamic links to use with the linker. Can be the name of the source target, a link identifier (no extension), or the full relative path to a static or dynamic library.",
			"minLength": 1
		})json"_ojson;
		links[SKeys::Pattern] = kPatternTargetSourceLinks;
		defs[Defs::TargetSourceCxxLinks] = makeArrayOrString(std::move(links));
	}

	defs[Defs::TargetSourceCxxMacOsFrameworkPaths] = makeArrayOrString(R"json({
		"type": "string",
		"description": "[deprecated: use appleFrameworkPaths]\n\nA list of paths to search for MacOS Frameworks",
		"minLength": 1
	})json"_ojson);

	defs[Defs::TargetSourceCxxMacOsFrameworks] = makeArrayOrString(R"json({
		"type": "string",
		"description": "[deprecated: use appleFrameworks]\n\nA list of MacOS Frameworks to link to the project.\n\nNote: Only the name of the framework is necessary (ex: 'Foundation' instead of Foundation.framework)",
		"minLength": 1
	})json"_ojson);

	defs[Defs::TargetSourceCxxAppleFrameworkPaths] = makeArrayOrString(R"json({
		"type": "string",
		"description": "A list of paths to search for Apple Frameworks",
		"minLength": 1
	})json"_ojson);

	defs[Defs::TargetSourceCxxAppleFrameworks] = makeArrayOrString(R"json({
		"type": "string",
		"description": "A list of Apple Frameworks to link to the project.\n\nNote: Only the name of the framework is necessary (ex: `Foundation` instead of `Foundation.framework`)",
		"minLength": 1
	})json"_ojson);

	defs[Defs::TargetSourceCxxPrecompiledHeader] = R"json({
		"type": "string",
		"description": "Treat a header file as a pre-compiled header and include it during compilation of every object file in the project. Define a path relative to the workspace root, but it must be contained within a sub-folder (such as `src`).",
		"minLength": 1
	})json"_ojson;

	defs[Defs::TargetSourceCxxInputCharSet] = R"json({
		"type": "string",
		"description": "The character set used by input source files in this target. Default: UTF-8",
		"minLength": 1,
		"default": "UTF-8"
	})json"_ojson;

	defs[Defs::TargetSourceCxxExecutionCharSet] = R"json({
		"type": "string",
		"description": "The execution character set to be given to the compiler. Default: UTF-8",
		"minLength": 1,
		"default": "UTF-8"
	})json"_ojson;

	defs[Defs::TargetSourceCxxThreads] = R"json({
		"type": "boolean",
		"description": "true to enable the preferred thread implementation of the compiler, such as pthreads (default), false to disable.",
		"default": true
	})json"_ojson;

	defs[Defs::TargetSourceCxxCppFilesystem] = R"json({
		"type": "boolean",
		"description": "true to enable C++17 filesystem in previous language standards (equivalent to `-lc++-fs`), false to disable (default).",
		"default": false
	})json"_ojson;

	defs[Defs::TargetSourceCxxCppModules] = R"json({
		"type": "boolean",
		"description": "true to enable C++20 modules, false to disable (default).",
		"default": false
	})json"_ojson;

	defs[Defs::TargetSourceCxxCppCoroutines] = R"json({
		"type": "boolean",
		"description": "true to enable C++20 coroutines (equivalent to `-fcoroutines` or `-fcoroutines-ts`), false to disable (default).",
		"default": false
	})json"_ojson;

	defs[Defs::TargetSourceCxxCppConcepts] = R"json({
		"type": "boolean",
		"description": "true to enable C++20 concepts in previous language standards (equivalent to `-fconcepts` or `-fconcepts-ts`), false to disable (default).",
		"default": false
	})json"_ojson;

	defs[Defs::TargetSourceCxxRuntimeTypeInfo] = R"json({
		"type": "boolean",
		"description": "true to include run-time type information (default), false to exclude.",
		"default": true
	})json"_ojson;

	defs[Defs::TargetSourceCxxPositionIndependent] = R"json({
		"description": "true to use position independent code. In MSVC, this does nothing. in GCC/Clang, `-fPIC` will be added to shared libraries and static libraries that link to shared libraries (within the workspace). `-fPIE` will be added to executables and static libraries that link to executables (within the workspace). Executables in GCC will be linked with `-pie`. This behavior can be set manually with `shared` or `executable` instead. false to disable (default).",
		"oneOf": [
			{
				"type": "boolean",
				"default": false
			},
			{
				"type": "string",
				"minLength": 1,
				"enum": [
					"shared",
					"executable"
				]
			}
		],
		"default": true
	})json"_ojson;

	defs[Defs::TargetSourceCxxFastMath] = R"json({
		"type": "boolean",
		"description": "true to enable additional (and potentially dangerous) floating point optimizations (equivalent to `-ffast-math`). false to disable (default).",
		"default": false
	})json"_ojson;

	defs[Defs::TargetSourceCxxExceptions] = R"json({
		"type": "boolean",
		"description": "true to use exceptions (default), false to turn off exceptions.",
		"default": true
	})json"_ojson;

	defs[Defs::TargetSourceCxxBuildSuffix] = R"json({
		"type": "string",
		"description": "Describes a suffix used to differentiate targets within build folders. If the same suffix is used between multiple targets, they can share objects and a precompiled header. By default, the suffix is the name of the target, so setting this to the name of another target will share its objects.\nAffected paths:\n`${buildOutputDir}/obj.${suffix}`\n`${buildOutputDir}/asm.${suffix}`\n`${buildDir}/int.${suffix}`",
		"minLength": 1
	})json"_ojson;
	defs[Defs::TargetSourceCxxBuildSuffix][SKeys::Pattern] = kPatternTargetName;

	defs[Defs::TargetSourceCxxStaticRuntimeLibrary] = R"json({
		"description": "true to statically link against compiler runtime libraries (libc++, MS UCRT, etc.). false to dynamically link them (default).",
		"type": "boolean",
		"default": false
	})json"_ojson;

	{
		Json staticLinks = R"json({
			"type": "string",
			"description": "A list of libraries to statically link with the linker. Can be the name of the source target, a link identifier (no extension), or the full relative path to a static library.",
			"minLength": 1
		})json"_ojson;
		staticLinks[SKeys::Pattern] = kPatternTargetSourceLinks;
		defs[Defs::TargetSourceCxxStaticLinks] = makeArrayOrString(std::move(staticLinks));
	}

	defs[Defs::TargetSourceCxxUnityBuild] = R"json({
		"description": "true to automatically build this target as a unity build. false to disable (default). This will combine all included source files into a single compilation unit in the order they're declared in `files`.",
		"type": "boolean",
		"default": false
	})json"_ojson;

	defs[Defs::TargetSourceCxxTreatWarningsAsErrors] = R"json({
		"description": "true to treat all warnings as errors. false to disable (default).",
		"type": "boolean",
		"default": false
	})json"_ojson;

	defs[Defs::TargetSourceCxxWarningsPreset] = R"json({
		"type": "string",
		"description": "Either a preset of the warnings to use, or the warnings flags themselves (excluding `-W` prefix)",
		"minLength": 1,
		"enum": [
			"none",
			"minimal",
			"pedantic",
			"strict",
			"strictPedantic",
			"veryStrict"
		],
		"default": "none"
	})json"_ojson;

	defs[Defs::TargetSourceCxxWarnings] = R"json({
		"type": "array",
		"description": "Either a preset of the warnings to use, or the warnings flags themselves (excluding `-W` prefix)",
		"uniqueItems": true,
		"minItems": 1,
		"items": {
			"type": "string",
			"minLength": 1
		}
	})json"_ojson;

	defs[Defs::TargetSourceCxxWarnings][SKeys::Items][SKeys::Examples] = {
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
		"description": "The subsystem to use for the target on Windows systems. If not specified, defaults to `console`",
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
		"description": "The type of entry point to use for the target on Windows systems. If not specified, defaults to `main`",
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

	defs[Defs::TargetSourceCxxMinGWUnixSharedLibraryNamingConvention] = R"json({
		"type": "boolean",
		"description": "If true (default), shared libraries will use the `lib(name).dll` naming convention in the MinGW toolchain (default), false to use `(name).dll`.",
		"default": true
	})json"_ojson;

	//

	defs[Defs::TargetScriptFile] = R"json({
		"description": "The relative path to a script file to run.",
		"type": "string",
		"minLength": 1
	})json"_ojson;

	defs[Defs::TargetScriptArguments] = makeArrayOrString(R"json({
		"type": "string",
		"description": "A list of arguments to pass along to the script.",
		"minLength": 1
	})json"_ojson,
		false);

	defs[Defs::TargetScriptDependsOn] = R"json({
		"type": "string",
		"description": "A target this script depends on in order to run.",
		"minLength": 1
	})json"_ojson;

	//

	defs[Defs::TargetProcessPath] = R"json({
		"type": "string",
		"description": "Either the full path to an exectuable, or a shell compatible name to be resolved.",
		"minLength": 1
	})json"_ojson;

	defs[Defs::TargetProcessArguments] = makeArrayOrString(R"json({
		"type": "string",
		"description": "A list of arguments to pass along to the process.",
		"minLength": 1
	})json"_ojson,
		false);

	defs[Defs::TargetProcessDependsOn] = R"json({
		"type": "string",
		"description": "A target this process depends on in order to run.",
		"minLength": 1
	})json"_ojson;

	//

	defs[Defs::TargetValidationSchema] = R"json({
		"type": "string",
		"description": "A JSON schema (Draft 7) to validate files against. File requires the '$schema' key/value.",
		"minLength": 1
	})json"_ojson;

	defs[Defs::TargetValidationFiles] = makeArrayOrString(R"json({
		"type": "string",
		"description": "File(s) to be validated using the selected schema.",
		"minLength": 1
	})json"_ojson);

	//

	defs[Defs::TargetCMakeLocation] = R"json({
		"type": "string",
		"description": "The folder path of the root CMakeLists.txt for the project.",
		"minLength": 1
	})json"_ojson;

	defs[Defs::TargetCMakeBuildFile] = R"json({
		"type": "string",
		"description": "The build file to use, if not CMakeLists.txt, relative to the location. (`-C` options)",
		"minLength": 1
	})json"_ojson;

	defs[Defs::TargetCMakeTargetNames] = makeArrayOrString(R"json({
		"type": "string",
		"description": "A specific CMake target, or targets to build instead of the default (all).",
		"minLength": 1
	})json"_ojson);

	defs[Defs::TargetCMakeDefines] = makeArrayOrString(R"json({
		"type": "string",
		"description": "Macro definitions to be passed into CMake. (`-D` options)",
		"minLength": 1
	})json"_ojson);

	defs[Defs::TargetCMakeRecheck] = R"json({
		"type": "boolean",
		"description": "If true (default), CMake will be invoked each time during the build.",
		"default": true
	})json"_ojson;

	defs[Defs::TargetCMakeRebuild] = R"json({
		"type": "boolean",
		"description": "If true (default), the CMake build folder will be cleaned and rebuilt when a rebuild is requested.",
		"default": true
	})json"_ojson;

	defs[Defs::TargetCMakeClean] = R"json({
		"type": "boolean",
		"description": "If true (default), the CMake build folder will be cleaned when a clean is requested.",
		"default": true
	})json"_ojson;

	defs[Defs::TargetCMakeToolset] = R"json({
		"type": "string",
		"description": "A toolset file to be passed to CMake (`-T` option).",
		"minLength": 1
	})json"_ojson;

	//
	defs[Defs::TargetChaletLocation] = R"json({
		"type": "string",
		"description": "",
		"minLength": 1
	})json"_ojson;
	defs[Defs::TargetChaletLocation]["description"] = fmt::format("The folder path of the root {} for the project.", m_inputs.defaultInputFile());

	defs[Defs::TargetChaletBuildFile] = R"json({
		"type": "string",
		"description": "",
		"minLength": 1
	})json"_ojson;
	defs[Defs::TargetChaletBuildFile]["description"] = fmt::format("The build file to use, if not {}, relative to the location.", m_inputs.defaultInputFile());

	defs[Defs::TargetChaletTargetNames] = makeArrayOrString(R"json({
		"type": "string",
		"description": "A specific Chalet target, or targets to build instead of the default (all).",
		"minLength": 1
	})json"_ojson);

	defs[Defs::TargetChaletRecheck] = R"json({
		"type": "boolean",
		"description": "If true (default), Chalet will be invoked each time during the build.",
		"default": true
	})json"_ojson;

	defs[Defs::TargetChaletRebuild] = R"json({
		"type": "boolean",
		"description": "If true (default), the Chalet build folder will be cleaned and rebuilt when a rebuild is requested.",
		"default": true
	})json"_ojson;

	defs[Defs::TargetChaletClean] = R"json({
		"type": "boolean",
		"description": "If true (default), the Chalet build folder will be cleaned when a clean is requested.",
		"default": true
	})json"_ojson;

	defs[Defs::TargetCMakeRunExecutable] = R"json({
		"type": "string",
		"description": "The path to an executable to run, relative to the build directory.",
		"minLength": 1
	})json"_ojson;

	//
	// Platform Requires
	//
	defs[Defs::PlatformRequiresUbuntuSystem] = makeArrayOrString(R"json({
		"type": "string",
		"description": "Ubuntu system packages to be checked before the build.",
		"minLength": 1
	})json"_ojson);

	defs[Defs::PlatformRequiresDebianSystem] = makeArrayOrString(R"json({
		"type": "string",
		"description": "Debian system packages to be checked before the build.",
		"minLength": 1
	})json"_ojson);

	defs[Defs::PlatformRequiresArchLinuxSystem] = makeArrayOrString(R"json({
		"type": "string",
		"description": "Arch Linux system packages to be checked before the build.",
		"minLength": 1
	})json"_ojson);

	defs[Defs::PlatformRequiresManjaroSystem] = makeArrayOrString(R"json({
		"type": "string",
		"description": "Manjaro system packages to be checked before the build.",
		"minLength": 1
	})json"_ojson);

	defs[Defs::PlatformRequiresFedoraSystem] = makeArrayOrString(R"json({
		"type": "string",
		"description": "Fedora system packages to be checked before the build.",
		"minLength": 1
	})json"_ojson);

	defs[Defs::PlatformRequiresRedHatSystem] = makeArrayOrString(R"json({
		"type": "string",
		"description": "Red Hat system packages to be checked before the build.",
		"minLength": 1
	})json"_ojson);

	defs[Defs::PlatformRequiresWindowsMSYS2] = makeArrayOrString(R"json({
		"type": "string",
		"description": "Windows MSYS2 packages to be checked before the build.",
		"minLength": 1
	})json"_ojson);

	defs[Defs::PlatformRequiresMacosMacPorts] = makeArrayOrString(R"json({
		"type": "string",
		"description": "MacOS MacPorts packages to be checked before the build.",
		"minLength": 1
	})json"_ojson);

	defs[Defs::PlatformRequiresMacosHomebrew] = makeArrayOrString(R"json({
		"type": "string",
		"description": "MacOS Homebrew packages to be checked before the build.",
		"minLength": 1
	})json"_ojson);

	//
	// Complex Definitions
	//
	{
		auto configuration = R"json({
			"type": "object",
			"description": "Properties to describe a single build configuration type.",
			"additionalProperties": false
		})json"_ojson;
		addProperty(configuration, "debugSymbols", Defs::ConfigurationDebugSymbols);
		addProperty(configuration, "enableProfiling", Defs::ConfigurationEnableProfiling);
		addProperty(configuration, "interproceduralOptimization", Defs::ConfigurationInterproceduralOptimization);
		addProperty(configuration, "optimizationLevel", Defs::ConfigurationOptimizationLevel);
		addProperty(configuration, "sanitize", Defs::ConfigurationSanitize);
		defs[Defs::Configuration] = std::move(configuration);
	}

	{
		auto distTarget = R"json({
			"type": "object",
			"description": "Properties to describe an individual bundle.",
			"additionalProperties": false,
			"required": [
				"kind"
			]
		})json"_ojson;
		distTarget[SKeys::Properties] = Json::object();
		addProperty(distTarget, "buildTargets", Defs::DistributionBundleBuildTargets);
		addProperty(distTarget, "condition", Defs::DistributionCondition);
		addPropertyAndPattern(distTarget, "exclude", Defs::DistributionBundleExclude, kPatternConditions);
		addPropertyAndPattern(distTarget, "include", Defs::DistributionBundleInclude, kPatternConditions);
		addProperty(distTarget, "includeDependentSharedLibraries", Defs::DistributionBundleIncludeDependentSharedLibraries);
		addKind(distTarget, defs, Defs::DistributionKind, "bundle");
		addProperty(distTarget, "windows", Defs::DistributionBundleWindows, false);
		addProperty(distTarget, "linuxDesktopEntry", Defs::DistributionBundleLinuxDesktopEntry, false);
		addProperty(distTarget, "macosBundle", Defs::DistributionBundleMacOSBundle, false);
		addProperty(distTarget, "mainExecutable", Defs::DistributionBundleMainExecutable);
		addProperty(distTarget, "outputDescription", Defs::TargetOutputDescription);
		addProperty(distTarget, "subdirectory", Defs::DistributionBundleSubDirectory);
		defs[Defs::DistributionBundle] = std::move(distTarget);
	}

	{
		auto distArchive = R"json({
			"type": "object",
			"description": "Properties to describe an individual distribution archive.",
			"additionalProperties": false,
			"required": [
				"kind"
			]
		})json"_ojson;
		addProperty(distArchive, "condition", Defs::DistributionCondition);
		addPropertyAndPattern(distArchive, "format", Defs::DistributionArchiveFormat, kPatternConditions);
		addPropertyAndPattern(distArchive, "include", Defs::DistributionArchiveInclude, kPatternConditions);
		addKind(distArchive, defs, Defs::DistributionKind, "archive");
		addProperty(distArchive, "outputDescription", Defs::TargetOutputDescription);
		addProperty(distArchive, "macosNotarizationProfile", Defs::DistributionArchiveMacosNotarizationProfile);
		// macosNotarizationProfile

		defs[Defs::DistributionArchive] = std::move(distArchive);
	}

	{
		auto distMacosDiskImage = R"json({
			"type": "object",
			"description": "Properties to describe a macos disk image (dmg). Implies 'condition: macos'",
			"additionalProperties": false,
			"required": [
				"kind",
				"size",
				"positions"
			]
		})json"_ojson;
		distMacosDiskImage[SKeys::Properties]["background"] = defs[Defs::DistributionMacosDiskImageBackground];
		addProperty(distMacosDiskImage, "iconSize", Defs::DistributionMacosDiskImageIconSize);
		addKind(distMacosDiskImage, defs, Defs::DistributionKind, "macosDiskImage");
		addProperty(distMacosDiskImage, "outputDescription", Defs::TargetOutputDescription);
		addProperty(distMacosDiskImage, "pathbarVisible", Defs::DistributionMacosDiskImagePathbarVisible);
		addProperty(distMacosDiskImage, "positions", Defs::DistributionMacosDiskImagePositions, false);
		addProperty(distMacosDiskImage, "size", Defs::DistributionMacosDiskImageSize, false);
		addProperty(distMacosDiskImage, "textSize", Defs::DistributionMacosDiskImageTextSize);
		defs[Defs::DistributionMacosDiskImage] = std::move(distMacosDiskImage);
	}
	{
		auto distScript = R"json({
			"type": "object",
			"description": "Run a single script.",
			"additionalProperties": false,
			"required": [
				"kind"
			]
		})json"_ojson;
		addPropertyAndPattern(distScript, "arguments", Defs::TargetScriptArguments, kPatternConditions);
		addProperty(distScript, "condition", Defs::DistributionCondition);
		addPropertyAndPattern(distScript, "dependsOn", Defs::TargetScriptDependsOn, kPatternConditions);
		addKind(distScript, defs, Defs::DistributionKind, "script");
		addPropertyAndPattern(distScript, "file", Defs::TargetScriptFile, kPatternConditions);
		addProperty(distScript, "outputDescription", Defs::TargetOutputDescription);
		defs[Defs::DistributionScript] = std::move(distScript);
	}
	{
		auto distProcess = R"json({
			"type": "object",
			"description": "Run a single process.",
			"additionalProperties": false,
			"required": [
				"kind",
				"path"
			]
		})json"_ojson;
		addPropertyAndPattern(distProcess, "arguments", Defs::TargetProcessArguments, kPatternConditions);
		addProperty(distProcess, "condition", Defs::DistributionCondition);
		addPropertyAndPattern(distProcess, "dependsOn", Defs::TargetProcessDependsOn, kPatternConditions);
		addKind(distProcess, defs, Defs::DistributionKind, "process");
		addProperty(distProcess, "outputDescription", Defs::TargetOutputDescription);
		addPropertyAndPattern(distProcess, "path", Defs::TargetProcessPath, kPatternConditions);
		defs[Defs::DistributionProcess] = std::move(distProcess);
	}
	{
		auto distValidation = R"json({
			"type": "object",
			"description": "Validate JSON file(s) against a schema. Unlike with build validation target, all files will always validate.",
			"additionalProperties": false,
			"required": [
				"kind"
			]
		})json"_ojson;
		addProperty(distValidation, "condition", Defs::DistributionCondition);
		addPropertyAndPattern(distValidation, "files", Defs::TargetValidationFiles, kPatternConditions);
		addKind(distValidation, defs, Defs::DistributionKind, "validation");
		addProperty(distValidation, "outputDescription", Defs::TargetOutputDescription);
		addPropertyAndPattern(distValidation, "schema", Defs::TargetValidationSchema, kPatternConditions);
		defs[Defs::DistributionValidation] = std::move(distValidation);
	}

	{
		auto variables = R"json({
			"type": "object",
			"description": "Local variables to be used inside of the build file, but shouldn't be part of the environment (.env) - ie. shortcuts to paths that may otherwise be verbose."
		})json"_ojson;
		variables[SKeys::PatternProperties][R"(^[A-Za-z0-9_]{3,255}$)"] = getDefinition(Defs::EnvironmentVariableValue);
		defs[Defs::EnvironmentVariables] = std::move(variables);
	}

	{
		auto externalGit = R"json({
			"type": "object",
			"additionalProperties": false,
			"description": "An external git dependency",
			"required": [
				"kind",
				"repository"
			]
		})json"_ojson;
		externalGit[SKeys::Properties] = Json::object();
		addProperty(externalGit, "branch", Defs::ExternalDependencyGitBranch);
		addProperty(externalGit, "commit", Defs::ExternalDependencyGitCommit);
		addProperty(externalGit, "condition", Defs::ExternalDependencyCondition);
		addKind(externalGit, defs, Defs::ExternalDependencyKind, "git");
		addProperty(externalGit, "repository", Defs::ExternalDependencyGitRepository);
		addProperty(externalGit, "submodules", Defs::ExternalDependencyGitSubmodules);
		addProperty(externalGit, "tag", Defs::ExternalDependencyGitTag);
		defs[Defs::ExternalDependencyGit] = std::move(externalGit);
	}
	{
		auto externalLocal = R"json({
			"type": "object",
			"additionalProperties": false,
			"required": [
				"kind",
				"path"
			]
		})json"_ojson;
		addProperty(externalLocal, "condition", Defs::ExternalDependencyCondition);
		addKind(externalLocal, defs, Defs::ExternalDependencyKind, "local");
		addProperty(externalLocal, "path", Defs::ExternalDependencyLocalPath);
		defs[Defs::ExternalDependencyLocal] = std::move(externalLocal);
	}
	{
		auto externalScript = R"json({
			"type": "object",
			"additionalProperties": false,
			"required": [
				"kind",
				"file"
			]
		})json"_ojson;
		addProperty(externalScript, "arguments", Defs::TargetScriptArguments);
		addProperty(externalScript, "condition", Defs::ExternalDependencyCondition);
		addKind(externalScript, defs, Defs::ExternalDependencyKind, "script");
		addProperty(externalScript, "file", Defs::TargetScriptFile);
		defs[Defs::ExternalDependencyScript] = std::move(externalScript);
	}
	{
		auto sourceTargetCxx = R"json({
			"type": "object",
			"description": "Settings for compiling C, C++, and Windows resource files.\nMay also include settings related to linking.",
			"additionalProperties": false
		})json"_ojson;
		addPropertyAndPattern(sourceTargetCxx, "appleFrameworkPaths", Defs::TargetSourceCxxAppleFrameworkPaths, kPatternConditions);
		addPropertyAndPattern(sourceTargetCxx, "appleFrameworks", Defs::TargetSourceCxxAppleFrameworks, kPatternConditions);
		addProperty(sourceTargetCxx, "buildSuffix", Defs::TargetSourceCxxBuildSuffix);
		addPropertyAndPattern(sourceTargetCxx, "compileOptions", Defs::TargetSourceCxxCompileOptions, kPatternConditions);
		addPropertyAndPattern(sourceTargetCxx, "cppConcepts", Defs::TargetSourceCxxCppConcepts, kPatternConditions);
		addPropertyAndPattern(sourceTargetCxx, "cppCoroutines", Defs::TargetSourceCxxCppCoroutines, kPatternConditions);
		addPropertyAndPattern(sourceTargetCxx, "cppFilesystem", Defs::TargetSourceCxxCppFilesystem, kPatternConditions);
		addPropertyAndPattern(sourceTargetCxx, "cppModules", Defs::TargetSourceCxxCppModules, kPatternConditions);
		addPropertyAndPattern(sourceTargetCxx, "cppStandard", Defs::TargetSourceCxxCppStandard, kPatternConditions);
		addPropertyAndPattern(sourceTargetCxx, "cStandard", Defs::TargetSourceCxxCStandard, kPatternConditions);
		addPropertyAndPattern(sourceTargetCxx, "defines", Defs::TargetSourceCxxDefines, kPatternConditions);
		addPropertyAndPattern(sourceTargetCxx, "exceptions", Defs::TargetSourceCxxExceptions, kPatternConditions);
		addPropertyAndPattern(sourceTargetCxx, "executionCharset", Defs::TargetSourceCxxExecutionCharSet, kPatternConditions);
		addPropertyAndPattern(sourceTargetCxx, "fastMath", Defs::TargetSourceCxxFastMath, kPatternConditions);
		addPropertyAndPattern(sourceTargetCxx, "includeDirs", Defs::TargetSourceCxxIncludeDirs, kPatternConditions);
		addPropertyAndPattern(sourceTargetCxx, "inputCharset", Defs::TargetSourceCxxInputCharSet, kPatternConditions);
		addPropertyAndPattern(sourceTargetCxx, "libDirs", Defs::TargetSourceCxxLibDirs, kPatternConditions);
		addPropertyAndPattern(sourceTargetCxx, "linkerOptions", Defs::TargetSourceCxxLinkerOptions, kPatternConditions);
		addPropertyAndPattern(sourceTargetCxx, "links", Defs::TargetSourceCxxLinks, kPatternConditions);

		// deprecated
		addPropertyAndPattern(sourceTargetCxx, "macosFrameworkPaths", Defs::TargetSourceCxxMacOsFrameworkPaths, kPatternConditions);
		addPropertyAndPattern(sourceTargetCxx, "macosFrameworks", Defs::TargetSourceCxxMacOsFrameworks, kPatternConditions);

		addPropertyAndPattern(sourceTargetCxx, "mingwUnixSharedLibraryNamingConvention", Defs::TargetSourceCxxMinGWUnixSharedLibraryNamingConvention, kPatternConditions);
		addPropertyAndPattern(sourceTargetCxx, "positionIndependentCode", Defs::TargetSourceCxxPositionIndependent, kPatternConditions);
		addProperty(sourceTargetCxx, "precompiledHeader", Defs::TargetSourceCxxPrecompiledHeader);
		addPropertyAndPattern(sourceTargetCxx, "runtimeTypeInformation", Defs::TargetSourceCxxRuntimeTypeInfo, kPatternConditions);
		addPropertyAndPattern(sourceTargetCxx, "staticLinks", Defs::TargetSourceCxxStaticLinks, kPatternConditions);
		addPropertyAndPattern(sourceTargetCxx, "staticRuntimeLibrary", Defs::TargetSourceCxxStaticRuntimeLibrary, kPatternConditions);
		addPropertyAndPattern(sourceTargetCxx, "threads", Defs::TargetSourceCxxThreads, kPatternConditions);
		addPropertyAndPattern(sourceTargetCxx, "treatWarningsAsErrors", Defs::TargetSourceCxxTreatWarningsAsErrors, kPatternConditions);
		addPropertyAndPattern(sourceTargetCxx, "unityBuild", Defs::TargetSourceCxxUnityBuild, kPatternConditions);
		addPropertyAndPattern(sourceTargetCxx, "warningsPreset", Defs::TargetSourceCxxWarningsPreset, kPatternConditions);
		addPropertyAndPattern(sourceTargetCxx, "warnings", Defs::TargetSourceCxxWarnings, kPatternConditions);
		// addProperty(sourceTargetCxx, "windowsOutputDef", Defs::TargetSourceCxxWindowsOutputDef);
		addPropertyAndPattern(sourceTargetCxx, "windowsApplicationIcon", Defs::TargetSourceCxxWindowsAppIcon, kPatternConditions);
		addPropertyAndPattern(sourceTargetCxx, "windowsApplicationManifest", Defs::TargetSourceCxxWindowsAppManifest, kPatternConditions);
		addPropertyAndPattern(sourceTargetCxx, "windowsEntryPoint", Defs::TargetSourceCxxWindowsEntryPoint, kPatternConditions);
		addPropertyAndPattern(sourceTargetCxx, "windowsSubSystem", Defs::TargetSourceCxxWindowsSubSystem, kPatternConditions);

		defs[Defs::TargetSourceCxx] = std::move(sourceTargetCxx);
	}

	{
		auto sourceMetadata = R"json({
			"description": "Metadata to assign to source targets that can be retrieved with the `PROJECT_` prefix within configure files.\n(See: `configureFiles`)",
			"oneOf": [
				{
					"type": "object",
					"additionalProperties": false
				},
				{
					"type": "string",
					"const": "workspace"
				}
			]
		})json"_ojson;
		addProperty(sourceMetadata[SKeys::OneOf][0], "author", Defs::TargetSourceMetadataAuthor);
		addProperty(sourceMetadata[SKeys::OneOf][0], "description", Defs::TargetSourceMetadataDescription);
		addProperty(sourceMetadata[SKeys::OneOf][0], "homepage", Defs::TargetSourceMetadataHomepage);
		addProperty(sourceMetadata[SKeys::OneOf][0], "license", Defs::TargetSourceMetadataLicense);
		addProperty(sourceMetadata[SKeys::OneOf][0], "name", Defs::TargetSourceMetadataName);
		addProperty(sourceMetadata[SKeys::OneOf][0], "readme", Defs::TargetSourceMetadataReadme);
		addProperty(sourceMetadata[SKeys::OneOf][0], "version", Defs::TargetSourceMetadataVersion);

		defs[Defs::TargetSourceMetadata] = std::move(sourceMetadata);
	}

	{
		auto abstractSource = R"json({
			"type": "object",
			"additionalProperties": false
		})json"_ojson;
		addProperty(abstractSource, "configureFiles", Defs::TargetSourceConfigureFiles);
		addPropertyAndPattern(abstractSource, "files", Defs::TargetSourceFiles, kPatternConditions);
		addPropertyAndPattern(abstractSource, "language", Defs::TargetSourceLanguage, kPatternConditions);
		addProperty(abstractSource, "metadata", Defs::TargetSourceMetadata);

		abstractSource[SKeys::Properties]["settings"] = R"json({
			"type": "object",
			"description": "Settings for each language",
			"additionalProperties": false
		})json"_ojson;
		abstractSource[SKeys::Properties]["settings"][SKeys::Properties]["Cxx"] = getDefinition(Defs::TargetSourceCxx);
		abstractSource[SKeys::Properties]["settings:Cxx"] = getDefinition(Defs::TargetSourceCxx);

		defs[Defs::TargetAbstract] = std::move(abstractSource);
	}
	{
		auto targetSource = R"json({
			"type": "object",
			"description": "Build a target from source files.",
			"additionalProperties": false,
			"required": [
				"kind"
			]
		})json"_ojson;
		addProperty(targetSource, "condition", Defs::TargetCondition);
		addProperty(targetSource, "configureFiles", Defs::TargetSourceConfigureFiles);
		addProperty(targetSource, "extends", Defs::TargetSourceExtends);
		addPropertyAndPattern(targetSource, "files", Defs::TargetSourceFiles, kPatternConditions);
		addKindEnum(targetSource, defs, Defs::TargetKind, { "staticLibrary", "sharedLibrary" });
		addPropertyAndPattern(targetSource, "language", Defs::TargetSourceLanguage, kPatternConditions);
		addProperty(targetSource, "metadata", Defs::TargetSourceMetadata);
		addProperty(targetSource, "outputDescription", Defs::TargetOutputDescription);

		const auto& abstractProperties = defs[Defs::TargetAbstract][SKeys::Properties];
		targetSource[SKeys::Properties]["settings"] = abstractProperties["settings"];
		targetSource[SKeys::Properties]["settings:Cxx"] = abstractProperties["settings:Cxx"];

		defs[Defs::TargetSourceLibrary] = std::move(targetSource);

		//
		defs[Defs::TargetSourceExecutable] = defs[Defs::TargetSourceLibrary];
		addKind(defs[Defs::TargetSourceExecutable], defs, Defs::TargetKind, "executable");
		addProperty(defs[Defs::TargetSourceExecutable], "defaultRunArguments", Defs::TargetDefaultRunArguments);
		addPropertyAndPattern(defs[Defs::TargetSourceExecutable], "copyFilesOnRun", Defs::TargetSourceCopyFilesOnRun, kPatternConditions);
	}

	{
		auto targetScript = R"json({
			"type": "object",
			"description": "Run a single script.",
			"additionalProperties": false,
			"required": [
				"kind"
			]
		})json"_ojson;
		addPropertyAndPattern(targetScript, "arguments", Defs::TargetScriptArguments, kPatternConditions);
		addProperty(targetScript, "condition", Defs::TargetCondition);
		addPropertyAndPattern(targetScript, "dependsOn", Defs::TargetScriptDependsOn, kPatternConditions);
		addKind(targetScript, defs, Defs::TargetKind, "script");
		// addProperty(targetScript, "defaultRunArguments", Defs::TargetDefaultRunArguments);
		addPropertyAndPattern(targetScript, "file", Defs::TargetScriptFile, kPatternConditions);
		addProperty(targetScript, "outputDescription", Defs::TargetOutputDescription);
		defs[Defs::TargetScript] = std::move(targetScript);
	}
	{
		auto targetProcess = R"json({
			"type": "object",
			"description": "Run a single process.",
			"additionalProperties": false,
			"required": [
				"kind",
				"path"
			]
		})json"_ojson;
		addPropertyAndPattern(targetProcess, "arguments", Defs::TargetProcessArguments, kPatternConditions);
		addProperty(targetProcess, "condition", Defs::TargetCondition);
		addPropertyAndPattern(targetProcess, "dependsOn", Defs::TargetProcessDependsOn, kPatternConditions);
		addKind(targetProcess, defs, Defs::TargetKind, "process");
		addProperty(targetProcess, "outputDescription", Defs::TargetOutputDescription);
		addPropertyAndPattern(targetProcess, "path", Defs::TargetProcessPath, kPatternConditions);
		defs[Defs::TargetProcess] = std::move(targetProcess);
	}
	{
		auto targetValidation = R"json({
			"type": "object",
			"description": "Validate JSON file(s) against a schema.",
			"additionalProperties": false,
			"required": [
				"kind"
			]
		})json"_ojson;
		addProperty(targetValidation, "condition", Defs::TargetCondition);
		addPropertyAndPattern(targetValidation, "files", Defs::TargetValidationFiles, kPatternConditions);
		addKind(targetValidation, defs, Defs::TargetKind, "validation");
		addProperty(targetValidation, "outputDescription", Defs::TargetOutputDescription);
		addPropertyAndPattern(targetValidation, "schema", Defs::TargetValidationSchema, kPatternConditions);
		defs[Defs::TargetValidation] = std::move(targetValidation);
	}

	{
		auto targetCMake = R"json({
			"type": "object",
			"description": "Build target(s) utilizing CMake.",
			"additionalProperties": false,
			"required": [
				"kind",
				"location"
			]
		})json"_ojson;
		addPropertyAndPattern(targetCMake, "buildFile", Defs::TargetCMakeBuildFile, kPatternConditions);
		addProperty(targetCMake, "condition", Defs::TargetCondition);
		addProperty(targetCMake, "defaultRunArguments", Defs::TargetDefaultRunArguments);
		addPropertyAndPattern(targetCMake, "defines", Defs::TargetCMakeDefines, kPatternConditions);
		addKind(targetCMake, defs, Defs::TargetKind, "cmakeProject");
		addProperty(targetCMake, "location", Defs::TargetCMakeLocation);
		addProperty(targetCMake, "outputDescription", Defs::TargetOutputDescription);
		addProperty(targetCMake, "recheck", Defs::TargetCMakeRecheck);
		addProperty(targetCMake, "rebuild", Defs::TargetCMakeRebuild);
		addProperty(targetCMake, "clean", Defs::TargetCMakeClean);
		addPropertyAndPattern(targetCMake, "runExecutable", Defs::TargetCMakeRunExecutable, kPatternConditions);
		addPropertyAndPattern(targetCMake, "targets", Defs::TargetCMakeTargetNames, kPatternConditions);
		addPropertyAndPattern(targetCMake, "toolset", Defs::TargetCMakeToolset, kPatternConditions);
		defs[Defs::TargetCMake] = std::move(targetCMake);
	}

	{
		auto targetChalet = R"json({
			"type": "object",
			"description": "Build target(s) utilizing separate Chalet projects.",
			"additionalProperties": false,
			"required": [
				"kind",
				"location"
			]
		})json"_ojson;
		addPropertyAndPattern(targetChalet, "buildFile", Defs::TargetChaletBuildFile, kPatternConditions);
		addProperty(targetChalet, "condition", Defs::TargetCondition);
		addKind(targetChalet, defs, Defs::TargetKind, "chaletProject");
		addProperty(targetChalet, "location", Defs::TargetChaletLocation);
		addProperty(targetChalet, "outputDescription", Defs::TargetOutputDescription);
		addProperty(targetChalet, "recheck", Defs::TargetChaletRecheck);
		addProperty(targetChalet, "rebuild", Defs::TargetChaletRebuild);
		addProperty(targetChalet, "clean", Defs::TargetChaletClean);
		addPropertyAndPattern(targetChalet, "targets", Defs::TargetChaletTargetNames, kPatternConditions);
		defs[Defs::TargetChalet] = std::move(targetChalet);
	}

	{
		auto platformRequires = R"json({
			"type": "object",
			"description": "Define system packages to be verfied before the build begins.",
			"additionalProperties": false
		})json"_ojson;
		addPropertyAndPattern(platformRequires, "ubuntu.system", Defs::PlatformRequiresUbuntuSystem, kPatternConditions);
		addPropertyAndPattern(platformRequires, "debian.system", Defs::PlatformRequiresDebianSystem, kPatternConditions);
		addPropertyAndPattern(platformRequires, "archlinux.system", Defs::PlatformRequiresArchLinuxSystem, kPatternConditions);
		addPropertyAndPattern(platformRequires, "manjaro.system", Defs::PlatformRequiresManjaroSystem, kPatternConditions);
		addPropertyAndPattern(platformRequires, "fedora.system", Defs::PlatformRequiresFedoraSystem, kPatternConditions);
		addPropertyAndPattern(platformRequires, "redhat.system", Defs::PlatformRequiresRedHatSystem, kPatternConditions);
		addPropertyAndPattern(platformRequires, "macos.macports", Defs::PlatformRequiresMacosMacPorts, kPatternConditions);
		addPropertyAndPattern(platformRequires, "macos.homebrew", Defs::PlatformRequiresMacosHomebrew, kPatternConditions);
		addPropertyAndPattern(platformRequires, "windows.msys2", Defs::PlatformRequiresWindowsMSYS2, kPatternConditions);

		defs[Defs::PlatformRequires] = std::move(platformRequires);
	}

	return defs;
}

/*****************************************************************************/
std::string ChaletJsonSchema::getDefinitionName(const Defs inDef)
{
	switch (inDef)
	{
		case Defs::WorkspaceName: return "workspace-name";
		case Defs::WorkspaceVersion: return "workspace-version";
		case Defs::WorkspaceDescription: return "workspace-description";
		case Defs::WorkspaceHomepage: return "workspace-homepage";
		case Defs::WorkspaceAuthor: return "workspace-author";
		case Defs::WorkspaceLicense: return "workspace-license";
		case Defs::WorkspaceReadme: return "workspace-readme";
		//
		case Defs::Configuration: return "configuration";
		case Defs::ConfigurationDebugSymbols: return "configuration-debugSymbols";
		case Defs::ConfigurationEnableProfiling: return "configuration-enableProfiling";
		case Defs::ConfigurationInterproceduralOptimization: return "configuration-interproceduralOptimization";
		case Defs::ConfigurationOptimizationLevel: return "configuration-optimizationLevel";
		case Defs::ConfigurationSanitize: return "configuration-sanitize";
		//
		case Defs::DistributionKind: return "dist-kind";
		case Defs::DistributionCondition: return "dist-condition";
		//
		case Defs::DistributionBundle: return "dist-bundle";
		case Defs::DistributionBundleInclude: return "dist-bundle-include";
		case Defs::DistributionBundleExclude: return "dist-bundle-exclude";
		case Defs::DistributionBundleMainExecutable: return "dist-bundle-mainExecutable";
		case Defs::DistributionBundleSubDirectory: return "dist-bundle-subdirectory";
		case Defs::DistributionBundleBuildTargets: return "dist-bundle-buildTargets";
		case Defs::DistributionBundleIncludeDependentSharedLibraries: return "dist-bundle-includeDependentSharedLibraries";
		case Defs::DistributionBundleWindows: return "dist-bundle-windows";
		case Defs::DistributionBundleMacOSBundle: return "dist-bundle-macosBundle";
		case Defs::DistributionBundleLinuxDesktopEntry: return "dist-bundle-linuxDesktopEntry";
		//
		case Defs::DistributionScript: return "dist-script";
		//
		case Defs::DistributionProcess: return "dist-process";
		//
		case Defs::DistributionValidation: return "dist-validation";
		//
		case Defs::DistributionArchive: return "dist-archive";
		case Defs::DistributionArchiveInclude: return "dist-archive-include";
		case Defs::DistributionArchiveFormat: return "dist-archive-format";
		case Defs::DistributionArchiveMacosNotarizationProfile: return "dist-archive-macosNotarizationProfile";
		//
		case Defs::DistributionMacosDiskImage: return "dist-macos-disk-image";
		case Defs::DistributionMacosDiskImagePathbarVisible: return "dist-macos-disk-image-pathbarVisible";
		case Defs::DistributionMacosDiskImageIconSize: return "dist-macos-disk-image-iconSize";
		case Defs::DistributionMacosDiskImageTextSize: return "dist-macos-disk-image-textSize";
		case Defs::DistributionMacosDiskImageBackground: return "dist-macos-disk-image-background";
		case Defs::DistributionMacosDiskImageSize: return "dist-macos-disk-image-size";
		case Defs::DistributionMacosDiskImagePositions: return "dist-macos-disk-image-positions";
		//
		case Defs::ExternalDependency: return "external-dependency";
		case Defs::ExternalDependencyKind: return "external-dependency-kind";
		case Defs::ExternalDependencyCondition: return "external-dependency-condition";
		//
		case Defs::ExternalDependencyGit: return "external-dependency-git";
		case Defs::ExternalDependencyGitRepository: return "external-dependency-git-repository";
		case Defs::ExternalDependencyGitBranch: return "external-dependency-git-branch";
		case Defs::ExternalDependencyGitCommit: return "external-dependency-git-commit";
		case Defs::ExternalDependencyGitTag: return "external-dependency-git-tag";
		case Defs::ExternalDependencyGitSubmodules: return "external-dependency-git-submodules";
		//
		case Defs::ExternalDependencyLocal: return "external-dependency-local";
		case Defs::ExternalDependencyLocalPath: return "external-dependency-local-path";
		//
		case Defs::ExternalDependencyScript: return "external-dependency-script";
		//
		case Defs::EnvironmentVariables: return "variables";
		case Defs::EnvironmentVariableValue: return "variable-value";
		case Defs::EnvironmentSearchPaths: return "searchPaths";
		//
		case Defs::TargetOutputDescription: return "target-outputDescription";
		case Defs::TargetKind: return "target-kind";
		case Defs::TargetCondition: return "target-condition";
		case Defs::TargetDefaultRunArguments: return "target-defaultRunArguments";
		case Defs::TargetSourceCopyFilesOnRun: return "target-copyFilesOnRun";
		//
		case Defs::TargetSourceExtends: return "target-source-extends";
		case Defs::TargetSourceFiles: return "target-source-files";
		case Defs::TargetSourceLanguage: return "target-source-language";
		case Defs::TargetSourceConfigureFiles: return "target-source-configureFiles";
		//
		case Defs::TargetAbstract: return "target-abstract";
		case Defs::TargetSourceExecutable: return "target-source-executable";
		case Defs::TargetSourceLibrary: return "target-source-library";
		//
		case Defs::TargetSourceMetadata: return "target-source-metadata";
		case Defs::TargetSourceMetadataName: return "target-source-metadata-name";
		case Defs::TargetSourceMetadataVersion: return "target-source-metadata-version";
		case Defs::TargetSourceMetadataDescription: return "target-source-metadata-description";
		case Defs::TargetSourceMetadataHomepage: return "target-source-metadata-homepage";
		case Defs::TargetSourceMetadataAuthor: return "target-source-metadata-author";
		case Defs::TargetSourceMetadataLicense: return "target-source-metadata-license";
		case Defs::TargetSourceMetadataReadme: return "target-source-metadata-readme";
		//
		case Defs::TargetSourceCxx: return "target-source-cxx";
		case Defs::TargetSourceCxxCStandard: return "target-source-cxx-cStandard";
		case Defs::TargetSourceCxxCppStandard: return "target-source-cxx-cppStandard";
		case Defs::TargetSourceCxxCompileOptions: return "target-source-cxx-compileOptions";
		case Defs::TargetSourceCxxLinkerOptions: return "target-source-cxx-linkerOptions";
		case Defs::TargetSourceCxxDefines: return "target-source-cxx-defines";
		case Defs::TargetSourceCxxIncludeDirs: return "target-source-cxx-includeDirs";
		case Defs::TargetSourceCxxLibDirs: return "target-source-cxx-libDirs";
		case Defs::TargetSourceCxxLinks: return "target-source-cxx-links";
		case Defs::TargetSourceCxxMacOsFrameworkPaths: return "target-source-cxx-macosFrameworkPaths";
		case Defs::TargetSourceCxxMacOsFrameworks: return "target-source-cxx-macosFrameworks";
		case Defs::TargetSourceCxxAppleFrameworkPaths: return "target-source-cxx-appleFrameworkPaths";
		case Defs::TargetSourceCxxAppleFrameworks: return "target-source-cxx-appleFrameworks";
		case Defs::TargetSourceCxxPrecompiledHeader: return "target-source-cxx-precompiledHeader";
		case Defs::TargetSourceCxxInputCharSet: return "target-source-cxx-inputCharset";
		case Defs::TargetSourceCxxExecutionCharSet: return "target-source-cxx-executionCharset";
		case Defs::TargetSourceCxxThreads: return "target-source-cxx-threads";
		case Defs::TargetSourceCxxCppFilesystem: return "target-source-cxx-cppFilesystem";
		case Defs::TargetSourceCxxCppModules: return "target-source-cxx-cppModules";
		case Defs::TargetSourceCxxCppCoroutines: return "target-source-cxx-cppCoroutines";
		case Defs::TargetSourceCxxCppConcepts: return "target-source-cxx-cppConcepts";
		case Defs::TargetSourceCxxRuntimeTypeInfo: return "target-source-cxx-runtimeTypeInformation";
		case Defs::TargetSourceCxxPositionIndependent: return "target-source-cxx-positionIndependentCode";
		case Defs::TargetSourceCxxFastMath: return "target-source-cxx-fastMath";
		case Defs::TargetSourceCxxExceptions: return "target-source-cxx-exceptions";
		case Defs::TargetSourceCxxBuildSuffix: return "target-source-cxx-buildSuffix";
		case Defs::TargetSourceCxxStaticRuntimeLibrary: return "target-source-cxx-staticRuntimeLibrary";
		case Defs::TargetSourceCxxStaticLinks: return "target-source-cxx-staticLinks";
		case Defs::TargetSourceCxxUnityBuild: return "target-source-cxx-unityBuild";
		case Defs::TargetSourceCxxWarnings: return "target-source-cxx-warnings";
		case Defs::TargetSourceCxxWarningsPreset: return "target-source-cxx-warningsPreset";
		case Defs::TargetSourceCxxTreatWarningsAsErrors: return "target-source-cxx-treatWarningsAsErrors";
		case Defs::TargetSourceCxxWindowsAppManifest: return "target-source-cxx-windowsApplicationManifest";
		case Defs::TargetSourceCxxWindowsAppIcon: return "target-source-cxx-windowsAppIcon";
		case Defs::TargetSourceCxxMinGWUnixSharedLibraryNamingConvention: return "target-source-cxx-mingwUnixSharedLibraryNamingConvention";
		// case Defs::TargetSourceCxxWindowsOutputDef: return "target-source-cxx-windowsOutputDef";
		case Defs::TargetSourceCxxWindowsSubSystem: return "target-source-cxx-windowsSubSystem";
		case Defs::TargetSourceCxxWindowsEntryPoint: return "target-source-cxx-windowsEntryPoint";
		//
		case Defs::TargetScript: return "target-script";
		case Defs::TargetScriptFile: return "target-script-file";
		case Defs::TargetScriptArguments: return "target-script-arguments";
		case Defs::TargetScriptDependsOn: return "target-script-dependsOn";
		//
		case Defs::TargetProcess: return "target-process";
		case Defs::TargetProcessPath: return "target-process-path";
		case Defs::TargetProcessArguments: return "target-process-arguments";
		case Defs::TargetProcessDependsOn: return "target-process-dependsOn";
		//
		case Defs::TargetValidation: return "target-validation";
		case Defs::TargetValidationSchema: return "target-validation-schema";
		case Defs::TargetValidationFiles: return "target-validation-files";
		//
		case Defs::TargetCMake: return "target-cmake";
		case Defs::TargetCMakeLocation: return "target-cmake-location";
		case Defs::TargetCMakeBuildFile: return "target-cmake-buildFile";
		case Defs::TargetCMakeDefines: return "target-cmake-defines";
		case Defs::TargetCMakeRecheck: return "target-cmake-recheck";
		case Defs::TargetCMakeRebuild: return "target-cmake-rebuild";
		case Defs::TargetCMakeClean: return "target-cmake-clean";
		case Defs::TargetCMakeTargetNames: return "target-cmake-targets";
		case Defs::TargetCMakeToolset: return "target-cmake-toolset";
		case Defs::TargetCMakeRunExecutable: return "target-cmake-runExecutable";
		//
		case Defs::TargetChalet: return "target-chalet";
		case Defs::TargetChaletLocation: return "target-chalet-location";
		case Defs::TargetChaletBuildFile: return "target-chalet-buildFile";
		case Defs::TargetChaletTargetNames: return "target-chalet-targets";
		case Defs::TargetChaletRecheck: return "target-chalet-recheck";
		case Defs::TargetChaletRebuild: return "target-chalet-rebuild";
		case Defs::TargetChaletClean: return "target-chalet-clean";
		//
		case Defs::PlatformRequires: return "platform-requires";
		case Defs::PlatformRequiresUbuntuSystem: return "platform-requires-ubuntu-system";
		case Defs::PlatformRequiresDebianSystem: return "platform-requires-debian-system";
		case Defs::PlatformRequiresArchLinuxSystem: return "platform-requires-archlinux-system";
		case Defs::PlatformRequiresManjaroSystem: return "platform-requires-manjaro-system";
		case Defs::PlatformRequiresFedoraSystem: return "platform-requires-fedora-system";
		case Defs::PlatformRequiresRedHatSystem: return "platform-requires-redhat-system";
		case Defs::PlatformRequiresWindowsMSYS2: return "platform-requires-windows-msys2";
		case Defs::PlatformRequiresMacosMacPorts: return "platform-requires-macos-macports";
		case Defs::PlatformRequiresMacosHomebrew: return "platform-requires-macos-homebrew";

		default: break;
	}

	chalet_assert(false, "Missing definition name");

	return std::string();
}

/*****************************************************************************/
Json ChaletJsonSchema::getDefinition(const Defs inDef)
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
Json ChaletJsonSchema::makeArrayOrString(const Json inString, const bool inUniqueItems)
{
	Json ret = R"json({
		"description": "",
		"oneOf": [
			{},
			{
				"type": "array",
				"uniqueItems": true,
				"minItems": 1,
				"items": {}
			}
		]
	})json"_ojson;
	ret[SKeys::OneOf][0] = inString;
	ret[SKeys::OneOf][1][SKeys::UniqueItems] = inUniqueItems;
	ret[SKeys::OneOf][1][SKeys::Items] = inString;
	if (inString.contains(SKeys::Description))
	{
		ret[SKeys::Description] = inString.at(SKeys::Description);
		ret[SKeys::OneOf][0].erase(SKeys::Description);
		ret[SKeys::OneOf][1][SKeys::Items].erase(SKeys::Description);
	}

	return ret;
}

/*****************************************************************************/
void ChaletJsonSchema::addProperty(Json& outJson, const char* inKey, const Defs inDef, const bool inIndexed)
{
	if (inIndexed)
		outJson[SKeys::Properties][inKey] = getDefinition(inDef);
	else
		outJson[SKeys::Properties][inKey] = m_nonIndexedDefs[inDef];
}

/*****************************************************************************/
void ChaletJsonSchema::addPropertyAndPattern(Json& outJson, const char* inKey, const Defs inDef, const std::string& inPattern)
{
	outJson[SKeys::Properties][inKey] = getDefinition(inDef);
	outJson[SKeys::PatternProperties][fmt::format("^{}{}$", inKey, inPattern)] = outJson[SKeys::Properties][inKey];
}

/*****************************************************************************/
void ChaletJsonSchema::addPatternProperty(Json& outJson, const char* inKey, const Defs inDef, const std::string& inPattern)
{
	outJson[SKeys::PatternProperties][fmt::format("^{}{}$", inKey, inPattern)] = getDefinition(inDef);
}

/*****************************************************************************/
void ChaletJsonSchema::addKind(Json& outJson, const DefinitionMap& inDefs, const Defs inDef, std::string&& inConst)
{
	outJson[SKeys::Properties]["kind"] = inDefs.at(inDef);
	outJson[SKeys::Properties]["kind"][SKeys::Const] = inConst;
}

/*****************************************************************************/
void ChaletJsonSchema::addKindEnum(Json& outJson, const DefinitionMap& inDefs, const Defs inDef, StringList&& inEnums)
{
	outJson[SKeys::Properties]["kind"] = inDefs.at(inDef);
	outJson[SKeys::Properties]["kind"][SKeys::Enum] = inEnums;
}

/*****************************************************************************/
Json ChaletJsonSchema::get()
{
	Json ret;
	ret["$schema"] = "http://json-schema.org/draft-07/schema";
	ret["type"] = "object";
	ret["additionalProperties"] = false;
	ret["required"] = {
		"name",
		"version",
		"targets"
	};

	if (m_defs.empty())
		m_defs = getDefinitions();

	if (m_useRefs)
	{
		ret[SKeys::Definitions] = Json::object();
		for (auto& [def, defJson] : m_defs)
		{
			const auto name = getDefinitionName(def);
			ret[SKeys::Definitions][name] = defJson;
		}
	}

	//
	ret[SKeys::Properties] = Json::object();
	ret[SKeys::PatternProperties] = Json::object();

	ret[SKeys::Properties]["author"] = getDefinition(Defs::WorkspaceAuthor);
	ret[SKeys::Properties]["description"] = getDefinition(Defs::WorkspaceDescription);
	ret[SKeys::Properties]["homepage"] = getDefinition(Defs::WorkspaceHomepage);
	ret[SKeys::Properties]["license"] = getDefinition(Defs::WorkspaceLicense);
	ret[SKeys::Properties]["name"] = getDefinition(Defs::WorkspaceName);
	ret[SKeys::Properties]["readme"] = getDefinition(Defs::WorkspaceReadme);
	ret[SKeys::Properties]["version"] = getDefinition(Defs::WorkspaceVersion);

	ret[SKeys::Properties]["platformRequires"] = getDefinition(Defs::PlatformRequires);

	ret[SKeys::PatternProperties][fmt::format("^abstracts:(\\*|{})$", kPatternAbstractName)] = getDefinition(Defs::TargetAbstract);
	ret[SKeys::PatternProperties][fmt::format("^abstracts:(\\*|{})$", kPatternAbstractName)][SKeys::Description] = "An abstract build target. 'abstracts:*' is a special target that gets implicitely added to each project";

	ret[SKeys::Properties]["abstracts"] = R"json({
		"type": "object",
		"additionalProperties": false,
		"description": "A list of abstract build targets"
	})json"_ojson;
	ret[SKeys::Properties]["abstracts"][SKeys::PatternProperties][fmt::format("^(\\*|{})$", kPatternAbstractName)] = getDefinition(Defs::TargetAbstract);
	ret[SKeys::Properties]["abstracts"][SKeys::PatternProperties][fmt::format("^(\\*|{})$", kPatternAbstractName)][SKeys::Description] = "An abstract build target. '*' is a special target that gets implicitely added to each project.";

	ret[SKeys::Properties]["allowedArchitectures"] = R"json({
		"type": "array",
		"description": "An array of allowed target architecture triples supported by the project. Use this to limit which ones can be used to build the project.",
		"uniqueItems": true,
		"default": [],
		"items": {
			"type": "string",
			"minLength": 1
		}
	})json"_ojson;

	ret[SKeys::Properties]["configurations"] = R"json({
		"type": "object",
		"description": "An object of custom build configurations. If one has the same name as a default build configuration, the default will be replaced.",
		"additionalProperties": false
	})json"_ojson;
	ret[SKeys::Properties]["configurations"][SKeys::PatternProperties][R"(^[A-Za-z]{3,}$)"] = getDefinition(Defs::Configuration);

	ret[SKeys::Properties]["defaultConfigurations"] = R"json({
		"type": "array",
		"description": "An array of allowed default build configuration names.",
		"uniqueItems": true,
		"default": [],
		"items": {
			"type": "string",
			"minLength": 1
		}
	})json"_ojson;
	ret[SKeys::Properties]["defaultConfigurations"][SKeys::Default] = BuildConfiguration::getDefaultBuildConfigurationNames();
	ret[SKeys::Properties]["defaultConfigurations"][SKeys::Items][SKeys::Enum] = BuildConfiguration::getDefaultBuildConfigurationNames();

	const auto distribution = "distribution";
	ret[SKeys::Properties][distribution] = R"json({
		"type": "object",
		"additionalProperties": false,
		"description": "A sequential list of distribution targets to be created during the bundle phase."
	})json"_ojson;
	ret[SKeys::Properties][distribution][SKeys::PatternProperties][kPatternDistributionName] = R"json({
		"type": "object",
		"description": "A single distribution target.",
		"properties": {},
		"oneOf": []
	})json"_ojson;
	ret[SKeys::Properties][distribution][SKeys::PatternProperties][kPatternDistributionName][SKeys::Properties]["kind"] = m_defs.at(Defs::DistributionKind);
	ret[SKeys::Properties][distribution][SKeys::PatternProperties][kPatternDistributionName][SKeys::Properties]["kind"]["enum"] = R"json([
		"bundle",
		"script",
		"process",
		"validation",
		"archive",
		"macosDiskImage"
	])json"_ojson;
	//
	ret[SKeys::Properties][distribution][SKeys::PatternProperties][kPatternDistributionName][SKeys::OneOf][0] = getDefinition(Defs::DistributionBundle);
	ret[SKeys::Properties][distribution][SKeys::PatternProperties][kPatternDistributionName][SKeys::OneOf][1] = getDefinition(Defs::DistributionScript);
	ret[SKeys::Properties][distribution][SKeys::PatternProperties][kPatternDistributionName][SKeys::OneOf][2] = getDefinition(Defs::DistributionArchive);
	ret[SKeys::Properties][distribution][SKeys::PatternProperties][kPatternDistributionName][SKeys::OneOf][3] = getDefinition(Defs::DistributionMacosDiskImage);
	ret[SKeys::Properties][distribution][SKeys::PatternProperties][kPatternDistributionName][SKeys::OneOf][4] = getDefinition(Defs::DistributionProcess);
	ret[SKeys::Properties][distribution][SKeys::PatternProperties][kPatternDistributionName][SKeys::OneOf][5] = getDefinition(Defs::DistributionValidation);

	ret[SKeys::Properties]["variables"] = getDefinition(Defs::EnvironmentVariables);

	const auto externalDependencies = "externalDependencies";
	const std::string patternExternalName{ "^[\\w\\-+.]{3,100}$" };
	ret[SKeys::Properties][externalDependencies] = R"json({
		"type": "object",
		"additionalProperties": false,
		"description": "Dependencies to resolve prior to building or via the configure command, that are considered external to this project. The object key will be used as a reference to the resulting location via '${external:(key)}'."
	})json"_ojson;
	ret[SKeys::Properties][externalDependencies][SKeys::PatternProperties][patternExternalName] = R"json({
		"type": "object",
		"description": "A single external dependency or script.",
		"properties": {},
		"oneOf": []
	})json"_ojson;
	ret[SKeys::Properties][externalDependencies][SKeys::PatternProperties][patternExternalName][SKeys::Properties]["kind"] = m_defs.at(Defs::ExternalDependencyKind);
	ret[SKeys::Properties][externalDependencies][SKeys::PatternProperties][patternExternalName][SKeys::Properties]["kind"]["enum"] = R"json([
		"git",
		"local",
		"script"
	])json"_ojson;
	ret[SKeys::Properties][externalDependencies][SKeys::PatternProperties][patternExternalName][SKeys::OneOf][0] = getDefinition(Defs::ExternalDependencyGit);
	ret[SKeys::Properties][externalDependencies][SKeys::PatternProperties][patternExternalName][SKeys::OneOf][1] = getDefinition(Defs::ExternalDependencyLocal);
	ret[SKeys::Properties][externalDependencies][SKeys::PatternProperties][patternExternalName][SKeys::OneOf][2] = getDefinition(Defs::ExternalDependencyScript);

	addPropertyAndPattern(ret, "searchPaths", Defs::EnvironmentSearchPaths, kPatternConditions);

	//
	const auto targets = "targets";
	ret[SKeys::Properties][targets] = R"json({
		"type": "object",
		"additionalProperties": false,
		"description": "A sequential list of build targets, cmake targets, or scripts."
	})json"_ojson;
	ret[SKeys::Properties][targets][SKeys::PatternProperties][kPatternTargetName] = R"json({
		"type": "object",
		"description": "A single build target or script.",
		"properties": {},
		"oneOf": []
	})json"_ojson;
	ret[SKeys::Properties][targets][SKeys::PatternProperties][kPatternTargetName][SKeys::Properties]["kind"] = m_defs.at(Defs::TargetKind);
	ret[SKeys::Properties][targets][SKeys::PatternProperties][kPatternTargetName][SKeys::Properties]["kind"]["enum"] = R"json([
		"staticLibrary",
		"sharedLibrary",
		"executable",
		"cmakeProject",
		"chaletProject",
		"script",
		"process",
		"validation"
	])json"_ojson;
	ret[SKeys::Properties][targets][SKeys::PatternProperties][kPatternTargetName][SKeys::OneOf][0] = getDefinition(Defs::TargetSourceExecutable);
	ret[SKeys::Properties][targets][SKeys::PatternProperties][kPatternTargetName][SKeys::OneOf][1] = getDefinition(Defs::TargetSourceLibrary);
	ret[SKeys::Properties][targets][SKeys::PatternProperties][kPatternTargetName][SKeys::OneOf][2] = getDefinition(Defs::TargetCMake);
	ret[SKeys::Properties][targets][SKeys::PatternProperties][kPatternTargetName][SKeys::OneOf][3] = getDefinition(Defs::TargetChalet);
	ret[SKeys::Properties][targets][SKeys::PatternProperties][kPatternTargetName][SKeys::OneOf][4] = getDefinition(Defs::TargetScript);
	ret[SKeys::Properties][targets][SKeys::PatternProperties][kPatternTargetName][SKeys::OneOf][5] = getDefinition(Defs::TargetProcess);
	ret[SKeys::Properties][targets][SKeys::PatternProperties][kPatternTargetName][SKeys::OneOf][6] = getDefinition(Defs::TargetValidation);

	return ret;
}
}
