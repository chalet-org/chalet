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
		EnumPlatform,
		//
		EnvironmentPath,
		//
		TargetDescription,
		TargetType,
		TargetNotInConfiguration,
		TargetNotInPlatform,
		TargetOnlyInConfiguration,
		TargetOnlyInPlatform,
		//
		ProjectTarget,
		ProjectTargetExtends,
		ProjectTargetFiles,
		ProjectTargetKind,
		ProjectTargetLocation,
		ProjectTargetLanguage,
		ProjectTargetRunProject,
		ProjectTargetRunArguments,
		ProjectTargetRunDependencies,
		//
		ProjectTargetCxx,
		ProjectTargetCxxCStandard,
		ProjectTargetCxxCppStandard,
		ProjectTargetCxxCompileOptions,
		ProjectTargetCxxDefines,
		ProjectTargetCxxIncludeDirs,
		ProjectTargetCxxLibDirs,
		ProjectTargetCxxLinkerScript,
		ProjectTargetCxxLinkerOptions,
		ProjectTargetCxxLinks,
		ProjectTargetCxxMacOsFrameworkPaths,
		ProjectTargetCxxMacOsFrameworks,
		ProjectTargetCxxObjectiveCxx,
		ProjectTargetCxxPrecompiledHeader,
		ProjectTargetCxxThreads,
		ProjectTargetCxxRunTimeTypeInfo,
		ProjectTargetCxxExceptions,
		ProjectTargetCxxStaticLinking,
		ProjectTargetCxxStaticLinks,
		ProjectTargetCxxWarnings,
		ProjectTargetCxxWindowsAppManifest,
		ProjectTargetCxxWindowsAppIcon,
		ProjectTargetCxxWindowsOutputDef,
		ProjectTargetCxxWindowsPrefixOutputFilename,
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
	const std::string kPatternConfigurations;
	const std::string kPatternPlatforms;

	DefinitionMap m_defs;
	bool m_useRefs = true;
};
}

#endif // CHALET_SCHEMA_BUILD_JSON_HPP
