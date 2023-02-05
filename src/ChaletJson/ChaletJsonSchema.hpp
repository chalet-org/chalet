/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_CHALET_JSON_SCHEMA_HPP
#define CHALET_CHALET_JSON_SCHEMA_HPP

#include "Libraries/Json.hpp"

namespace chalet
{
class ChaletJsonSchema
{
	enum class Defs : ushort
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
		DistributionBundleOutputDirectory,
		DistributionBundleBuildTargets,
		DistributionBundleIncludeDependentSharedLibraries,
		DistributionBundleMacOSBundle,
		DistributionBundleLinuxDesktopEntry,
		//
		DistributionScript,
		//
		DistributionArchive,
		DistributionArchiveFormat,
		DistributionArchiveInclude,
		//
		DistributionMacosDiskImage,
		DistributionMacosDiskImagePathbarVisible,
		DistributionMacosDiskImageIconSize,
		DistributionMacosDiskImageTextSize,
		DistributionMacosDiskImageBackground,
		DistributionMacosDiskImageSize,
		DistributionMacosDiskImagePositions,
		//
		DistributionProcess,
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
		ExternalDependencyScript,
		//
		// EnumPlatform,
		//
		Variables,
		VariableValue,
		EnvironmentSearchPaths,
		//
		TargetOutputDescription,
		TargetKind,
		TargetCondition,
		TargetRunTarget,
		TargetDefaultRunArguments,
		//
		TargetAbstract,
		TargetSourceExecutable,
		TargetSourceLibrary,
		TargetSourceExtends,
		TargetSourceFiles,
		TargetSourceLanguage,
		TargetSourceConfigureFiles,
		TargetSourceCopyFilesOnRun,
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
		TargetSourceCxxCStandard,
		TargetSourceCxxCppStandard,
		TargetSourceCxxCompileOptions,
		TargetSourceCxxDefines,
		TargetSourceCxxIncludeDirs,
		TargetSourceCxxLibDirs,
		TargetSourceCxxLinkerScript,
		TargetSourceCxxLinkerOptions,
		TargetSourceCxxLinks,
		TargetSourceCxxMacOsFrameworkPaths,
		TargetSourceCxxMacOsFrameworks,
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
		TargetSourceCxxWindowsSubSystem,
		TargetSourceCxxWindowsEntryPoint,
		//
		TargetScript,
		TargetScriptFile,
		TargetScriptArguments,
		//
		TargetCMake,
		TargetCMakeLocation,
		TargetCMakeBuildFile,
		TargetCMakeTargetNames,
		TargetCMakeDefines,
		TargetCMakeRecheck,
		TargetCMakeRebuild,
		TargetCMakeClean,
		TargetCMakeToolset,
		TargetCMakeRunExecutable,
		//
		TargetChalet,
		TargetChaletLocation,
		TargetChaletBuildFile,
		TargetChaletRecheck,
		TargetChaletRebuild,
		TargetChaletClean,
		//
		TargetProcess,
		TargetProcessPath,
		TargetProcessArguments,
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
	ChaletJsonSchema();

	Json get();

private:
	DefinitionMap getDefinitions();
	std::string getDefinitionName(const Defs inDef);
	Json getDefinition(const Defs inDef);
	Json makeArrayOrString(const Json inString, const bool inUniqueItems = true);

	void addProperty(Json& outJson, const char* inKey, const Defs inDef, const bool inIndexed = true);
	void addPropertyAndPattern(Json& outJson, const char* inKey, const Defs inDef, const std::string& inPattern);
	void addPatternProperty(Json& outJson, const char* inKey, const Defs inDef, const std::string& inPattern);
	void addKind(Json& outJson, const DefinitionMap& inDefs, const Defs inDef, std::string&& inConst);
	void addKindEnum(Json& outJson, const DefinitionMap& inDefs, const Defs inDef, StringList&& inEnums);

	//
	const std::string kPatternTargetName;
	const std::string kPatternAbstractName;
	const std::string kPatternTargetSourceLinks;
	const std::string kPatternDistributionName;
	const std::string kPatternDistributionNameSimple;
	const std::string kPatternVersion;
	const std::string kPatternConditions;

	DefinitionMap m_defs;
	DefinitionMap m_nonIndexedDefs;
	bool m_useRefs = true;
};
}

#endif // CHALET_CHALET_JSON_SCHEMA_HPP
