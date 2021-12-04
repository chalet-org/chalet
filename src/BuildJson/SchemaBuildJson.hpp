/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_SCHEMA_BUILD_JSON_HPP
#define CHALET_SCHEMA_BUILD_JSON_HPP

#include "Libraries/Json.hpp"

namespace chalet
{
class SchemaBuildJson
{
	enum class Defs : ushort
	{
		//
		Configuration,
		ConfigurationDebugSymbols,
		ConfigurationEnableProfiling,
		ConfigurationLinkTimeOptimizations,
		ConfigurationOptimizationLevel,
		ConfigurationStripSymbols,
		//
		DistributionTarget,
		DistributionTargetKind,
		DistributionTargetConfiguration,
		DistributionTargetExclude,
		DistributionTargetInclude,
		DistributionTargetIncludeDependentSharedLibraries,
		DistributionTargetLinux,
		DistributionTargetMacOS,
		DistributionTargetMainExecutable,
		DistributionTargetOutputDirectory,
		DistributionTargetBuildTargets,
		DistributionTargetWindows,
		//
		DistributionArchiveTarget,
		DistributionArchiveTargetInclude,
		//
		DistributionTargetCondition,
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
		TargetRunTargetArguments,
		TargetRunDependencies,
		//
		AbstractTarget,
		ExecutableSourceTarget,
		LibrarySourceTarget,
		SourceTargetExtends,
		SourceTargetFiles,
		SourceTargetLocation,
		SourceTargetLanguage,
		//
		SourceTargetCxx,
		SourceTargetCxxCStandard,
		SourceTargetCxxCppStandard,
		SourceTargetCxxCompileOptions,
		SourceTargetCxxDefines,
		SourceTargetCxxIncludeDirs,
		SourceTargetCxxLibDirs,
		SourceTargetCxxLinkerScript,
		SourceTargetCxxLinkerOptions,
		SourceTargetCxxLinks,
		SourceTargetCxxMacOsFrameworkPaths,
		SourceTargetCxxMacOsFrameworks,
		SourceTargetCxxPrecompiledHeader,
		SourceTargetCxxThreads,
		SourceTargetCxxCppModules,
		SourceTargetCxxCppCoroutines,
		SourceTargetCxxCppConcepts,
		SourceTargetCxxRunTimeTypeInfo,
		SourceTargetCxxExceptions,
		SourceTargetCxxStaticLinking,
		SourceTargetCxxStaticLinks,
		SourceTargetCxxWarnings,
		SourceTargetCxxWindowsAppManifest,
		SourceTargetCxxWindowsAppIcon,
		SourceTargetCxxWindowsOutputDef,
		SourceTargetCxxWindowsPrefixOutputFilename,
		SourceTargetCxxWindowsSubSystem,
		SourceTargetCxxWindowsEntryPoint,
		//
		BuildScriptTarget,
		DistributionScriptTarget,
		ScriptTargetScript,
		//
		CMakeTarget,
		CMakeTargetLocation,
		CMakeTargetBuildFile,
		CMakeTargetDefines,
		CMakeTargetRecheck,
		CMakeTargetToolset,
		CMakeTargetRunExecutable,
		//
		ChaletTarget,
		ChaletTargetLocation,
		ChaletTargetBuildFile,
		ChaletTargetRecheck,
	};
	using DefinitionMap = std::map<Defs, Json>;

public:
	SchemaBuildJson();

	Json get();

private:
	DefinitionMap getDefinitions();
	std::string getDefinitionName(const Defs inDef);
	Json getDefinition(const Defs inDef);

	const std::string kDefinitions;
	const std::string kItems;
	const std::string kProperties;
	const std::string kAdditionalProperties;
	const std::string kPattern;
	const std::string kPatternProperties;
	const std::string kDescription;
	const std::string kDefault;
	const std::string kEnum;
	const std::string kExamples;
	// const std::string kAnyOf;
	// const std::string kAllOf;
	const std::string kOneOf;
	const std::string kThen;
	const std::string kElse;

	//
	const std::string kPatternTargetName;
	const std::string kPatternAbstractName;
	const std::string kPatternSourceTargetLinks;
	const std::string kPatternDistributionName;
	const std::string kPatternConditionConfigurations;
	const std::string kPatternConditionPlatforms;
	const std::string kPatternConditionConfigurationsPlatforms;
	const std::string kPatternConditionPlatformsInner;
	const std::string kPatternConditionConfigurationsPlatformsInner;
	const std::string kPatternCompilers;

	DefinitionMap m_defs;
	bool m_useRefs = true;
};
}

#endif // CHALET_SCHEMA_BUILD_JSON_HPP
