/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include "Libraries/Json.hpp"

namespace chalet
{
struct CommandLineInputs;

class ChaletJsonSchema
{
	enum class Defs : u16
	{
		WorkspaceName,
		WorkspaceVersion,
		WorkspaceDescription,
		WorkspaceHomepage,
		WorkspaceAuthor,
		WorkspaceLicense,
		WorkspaceReadme,
		//
		Configuration,
		ConfigurationDebugSymbols,
		ConfigurationEnableProfiling,
		ConfigurationInterproceduralOptimization,
		ConfigurationOptimizationLevel,
		ConfigurationSanitize,
		//
		DistributionKind,
		DistributionCondition,
		//
		DistributionBundle,
		DistributionBundleExclude,
		DistributionBundleInclude,
		DistributionBundleMainExecutable,
		DistributionBundleSubDirectory,
		DistributionBundleBuildTargets,
		DistributionBundleIncludeDependentSharedLibraries,
		DistributionBundleWindows,
		DistributionBundleMacOSBundle,
		DistributionBundleLinuxDesktopEntry,
		//
		DistributionScript,
		DistributionScriptDependsOn,
		//
		DistributionProcess,
		DistributionProcessDependsOn,
		//
		DistributionValidation,
		//
		DistributionArchive,
		DistributionArchiveFormat,
		DistributionArchiveInclude,
		DistributionArchiveMacosNotarizationProfile,
		//
		DistributionMacosDiskImage,
		DistributionMacosDiskImagePathbarVisible,
		DistributionMacosDiskImageIconSize,
		DistributionMacosDiskImageTextSize,
		DistributionMacosDiskImageBackground,
		DistributionMacosDiskImageSize,
		DistributionMacosDiskImagePositions,
		//
		ExternalDependency,
		ExternalDependencyKind,
		ExternalDependencyCondition,
		//
		ExternalDependencyGit,
		ExternalDependencyGitRepository,
		ExternalDependencyGitBranch,
		ExternalDependencyGitCommit,
		ExternalDependencyGitTag,
		ExternalDependencyGitSubmodules,
		//
		ExternalDependencyLocal,
		ExternalDependencyLocalPath,
		//
		ExternalDependencyArchive,
		ExternalDependencyArchiveUrl,
		ExternalDependencyArchiveSubDirectory,
		//
		ExternalDependencyScript,
		//
		// EnumPlatform,
		//
		EnvironmentVariables,
		EnvironmentVariableValue,
		EnvironmentSearchPaths,
		EnvironmentPackagePaths,
		//
		TargetOutputDescription,
		TargetKind,
		TargetCondition,
		TargetDefaultRunArguments,
		TargetRunWorkingDirectory,
		//
		TargetAbstract,
		TargetSourceExecutable,
		TargetSourceLibrary,
		TargetSourceExtends,
		TargetSourceFiles,
		TargetSourceLanguage,
		TargetSourceConfigureFiles,
		TargetSourceCopyFilesOnRun,
		TargetSourceImportPackages,
		//
		TargetSourceMetadata,
		TargetSourceMetadataName,
		TargetSourceMetadataVersion,
		TargetSourceMetadataDescription,
		TargetSourceMetadataHomepage,
		TargetSourceMetadataAuthor,
		TargetSourceMetadataLicense,
		TargetSourceMetadataReadme,
		//
		TargetSourceCxx,
		TargetSourceCxxCcacheOptions,
		TargetSourceCxxCStandard,
		TargetSourceCxxCppStandard,
		TargetSourceCxxCompileOptions,
		TargetSourceCxxDefines,
		TargetSourceCxxIncludeDirs,
		TargetSourceCxxLibDirs,
		TargetSourceCxxLinkerOptions,
		TargetSourceCxxLinks,
		TargetSourceCxxAppleFrameworkPaths,
		TargetSourceCxxAppleFrameworks,
		TargetSourceCxxPrecompiledHeader,
		TargetSourceCxxThreads,
		TargetSourceCxxCppFilesystem,
		TargetSourceCxxCppModules,
		TargetSourceCxxCppCoroutines,
		TargetSourceCxxCppConcepts,
		TargetSourceCxxRuntimeTypeInfo,
		TargetSourceCxxPositionIndependent,
		TargetSourceCxxFastMath,
		TargetSourceCxxExceptions,
		TargetSourceCxxInputCharSet,
		TargetSourceCxxExecutionCharSet,
		TargetSourceCxxBuildSuffix,
		TargetSourceCxxStaticRuntimeLibrary,
		TargetSourceCxxStaticLinks,
		TargetSourceCxxUnityBuild,
		TargetSourceCxxWarningsPreset,
		TargetSourceCxxWarnings,
		TargetSourceCxxTreatWarningsAsErrors,
		TargetSourceCxxWindowsAppManifest,
		TargetSourceCxxWindowsAppIcon,
		TargetSourceCxxWindowsOutputDef,
		TargetSourceCxxMinGWUnixSharedLibraryNamingConvention,
		TargetSourceCxxJustMyCodeDebugging,
		TargetSourceCxxWindowsSubSystem,
		TargetSourceCxxWindowsEntryPoint,
		//
		TargetScript,
		TargetScriptFile,
		TargetScriptArguments,
		TargetScriptWorkingDirectory,
		TargetScriptDependsOn,
		TargetScriptDependsOnSelf,
		//
		TargetProcess,
		TargetProcessPath,
		TargetProcessArguments,
		TargetProcessWorkingDirectory,
		TargetProcessDependsOn,
		//
		TargetValidation,
		TargetValidationSchema,
		TargetValidationFiles,
		//
		TargetCMake,
		TargetCMakeLocation,
		TargetCMakeBuildFile,
		TargetCMakeTargetNames,
		TargetCMakeDefines,
		TargetCMakeRecheck,
		TargetCMakeRebuild,
		TargetCMakeClean,
		TargetCMakeInstall,
		TargetCMakeToolset,
		TargetCMakeRunExecutable,
		//
		TargetMeson,
		TargetMesonLocation,
		TargetMesonBuildFile,
		TargetMesonTargetNames,
		TargetMesonDefines,
		TargetMesonRecheck,
		TargetMesonRebuild,
		TargetMesonClean,
		TargetMesonInstall,
		TargetMesonRunExecutable,
		//
		TargetChalet,
		TargetChaletLocation,
		TargetChaletBuildFile,
		TargetChaletTargetNames,
		TargetChaletRecheck,
		TargetChaletRebuild,
		TargetChaletClean,
		//
		Package,
		// PackageSearchPaths,
		PackageSettingsCxx,
		//
		PlatformRequires,
		PlatformRequiresUbuntuSystem,
		PlatformRequiresDebianSystem,
		PlatformRequiresArchLinuxSystem,
		PlatformRequiresManjaroSystem,
		PlatformRequiresFedoraSystem,
		PlatformRequiresRedHatSystem,
		PlatformRequiresWindowsMSYS2,
		PlatformRequiresMacosMacPorts,
		PlatformRequiresMacosHomebrew,
	};
	using DefinitionMap = std::map<Defs, Json>;

public:
	static Json get(const CommandLineInputs& inInputs);

private:
	explicit ChaletJsonSchema(const CommandLineInputs& inInputs);

	Json get();

	DefinitionMap getDefinitions();
	std::string getDefinitionName(const Defs inDef);
	Json getDefinition(const Defs inDef);
	Json makeArrayOrString(const Json inString, const bool inUniqueItems = true);
	Json makeArrayStringOrObject(const Json inString, const bool inUniqueItems = true);

	void addProperty(Json& outJson, const char* inKey, const Defs inDef, const bool inIndexed = true);
	void addPropertyAndPattern(Json& outJson, const char* inKey, const Defs inDef, const std::string& inPattern);
	void addPatternProperty(Json& outJson, const char* inKey, const Defs inDef, const std::string& inPattern);
	void addKind(Json& outJson, const DefinitionMap& inDefs, const Defs inDef, std::string&& inConst);
	void addKindEnum(Json& outJson, const DefinitionMap& inDefs, const Defs inDef, StringList&& inEnums);

	const CommandLineInputs& m_inputs;

	//
	const std::string kPatternTargetName;
	const std::string kPatternAbstractName;
	const std::string kPatternPackageName;
	const std::string kPatternTargetSourceLinks;
	const std::string kPatternDistributionName;
	const std::string kPatternConditions;

	DefinitionMap m_defs;
	DefinitionMap m_nonIndexedDefs;
	bool m_useRefs = true;
};
}
