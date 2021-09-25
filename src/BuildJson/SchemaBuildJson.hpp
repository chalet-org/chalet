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
		DistributionTargetConfiguration,
		DistributionTargetExclude,
		DistributionTargetInclude,
		DistributionTargetIncludeDependentSharedLibraries,
		DistributionTargetLinux,
		DistributionTargetMacOS,
		DistributionTargetMainProject,
		DistributionTargetOutputDirectory,
		DistributionTargetProjects,
		DistributionTargetWindows,
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
		TargetType,
		TargetCondition,
		//
		SourceTarget,
		SourceTargetExtends,
		SourceTargetFiles,
		SourceTargetKind,
		SourceTargetLocation,
		SourceTargetLanguage,
		SourceTargetRunProject,
		SourceTargetRunArguments,
		SourceTargetRunDependencies,
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
		SourceTargetCxxObjectiveCxx,
		SourceTargetCxxPrecompiledHeader,
		SourceTargetCxxThreads,
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
		ScriptTarget,
		ScriptTargetScript,
		//
		CMakeTarget,
		CMakeTargetLocation,
		CMakeTargetBuildFile,
		CMakeTargetDefines,
		CMakeTargetRecheck,
		CMakeTargetToolset,
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
	const std::string kPattern;
	const std::string kPatternProperties;
	const std::string kDescription;
	// const std::string kEnum = "enum";
	const std::string kExamples;
	const std::string kAnyOf;
	const std::string kAllOf;
	const std::string kOneOf;

	//
	const std::string kPatternProjectName;
	const std::string kPatternProjectLinks;
	const std::string kPatternDistributionName;
	const std::string kPatternConditionConfigurations;
	const std::string kPatternConditionPlatforms;
	const std::string kPatternConditionConfigurationsPlatforms;
	const std::string kPatternConditionPlatformsInner;
	const std::string kPatternConditionConfigurationsPlatformsInner;

	DefinitionMap m_defs;
	bool m_useRefs = true;
};
}

#endif // CHALET_SCHEMA_BUILD_JSON_HPP
