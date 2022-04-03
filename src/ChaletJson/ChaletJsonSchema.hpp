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
		//
		Configuration,
		ConfigurationDebugSymbols,
		ConfigurationEnableProfiling,
		ConfigurationInterproceduralOptimization,
		ConfigurationOptimizationLevel,
		ConfigurationSanitize,
		//
		DistributionKind,
		DistributionConfiguration,
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
		DistributionWindowsNullsoftInstaller,
		DistributionWindowsNullsoftInstallerScript,
		DistributionWindowsNullsoftInstallerPluginDirs,
		DistributionWindowsNullsoftInstallerDefines,
		//
		DistributionProcess,
		//
		ExternalDependency,
		ExternalDependencyGitRepository,
		ExternalDependencyGitBranch,
		ExternalDependencyGitCommit,
		ExternalDependencyGitTag,
		ExternalDependencyGitSubmodules,
		//
		// EnumPlatform,
		//
		EnvironmentSearchPaths,
		//
		TargetDescription,
		TargetKind,
		TargetCondition,
		TargetRunTarget,
		TargetDefaultRunArguments,
		TargetCopyFilesOnRun,
		//
		TargetAbstract,
		TargetSourceExecutable,
		TargetSourceLibrary,
		TargetSourceExtends,
		TargetSourceFiles,
		TargetSourceLanguage,
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
		TargetSourceCxxRunTimeTypeInfo,
		TargetSourceCxxFastMath,
		TargetSourceCxxExceptions,
		TargetSourceCxxInputCharSet,
		TargetSourceCxxExecutionCharSet,
		TargetSourceCxxStaticRuntimeLibrary,
		TargetSourceCxxStaticLinks,
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
		//
		TargetCMake,
		TargetCMakeLocation,
		TargetCMakeBuildFile,
		TargetCMakeDefines,
		TargetCMakeRecheck,
		TargetCMakeRebuild,
		TargetCMakeToolset,
		TargetCMakeRunExecutable,
		//
		TargetChalet,
		TargetChaletLocation,
		TargetChaletBuildFile,
		TargetChaletRecheck,
		TargetChaletRebuild,
		//
		TargetProcess,
		TargetProcessPath,
		TargetProcessArguments
	};
	using DefinitionMap = std::map<Defs, Json>;

public:
	ChaletJsonSchema();

	Json get();

private:
	DefinitionMap getDefinitions();
	std::string getDefinitionName(const Defs inDef);
	Json getDefinition(const Defs inDef);
	Json makeArrayOrString(const Json inString);

	//
	const std::string kPatternTargetName;
	const std::string kPatternAbstractName;
	const std::string kPatternTargetSourceLinks;
	const std::string kPatternDistributionName;
	const std::string kPatternDistributionNameSimple;
	const std::string kPatternConditionConfigurations;
	const std::string kPatternDistributionPlatforms;
	const std::string kPatternConditionPlatforms;
	const std::string kPatternConditionConfigurationsPlatforms;
	const std::string kPatternDistributionCondition;
	const std::string kPatternTargetCondition;
	const std::string kPatternCompilers;
	const std::string kPatternVersion;

	DefinitionMap m_defs;
	DefinitionMap m_nonIndexedDefs;
	bool m_useRefs = true;
};
}

#endif // CHALET_CHALET_JSON_SCHEMA_HPP
