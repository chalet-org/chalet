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
		ConfigDebugSymbols,
		ConfigEnableProfiling,
		ConfigLinkTimeOptimizations,
		ConfigOptimizationLevel,
		ConfigStripSymbols,
		//
		Dist,
		DistConfiguration,
		DistDependencies,
		DistDescription,
		DistExclude,
		DistIncludeDependentSharedLibs,
		DistLinux,
		DistMacOS,
		DistMainProject,
		DistOutDirectory,
		DistProjects,
		DistWindows,
		//
		ExternalDependency,
		ExtGitRepository,
		ExtGitBranch,
		ExtGitCommit,
		ExtGitTag,
		ExtGitSubmodules,
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
		TargetProject,
		TargetProjectExtends,
		TargetProjectFiles,
		TargetProjectKind,
		TargetProjectLocation,
		TargetProjectLanguage,
		TargetProjectRunProject,
		TargetProjectRunArguments,
		TargetProjectRunDependencies,
		//
		TargetProjectCxx,
		TargetProjectCxxCStandard,
		TargetProjectCxxCppStandard,
		TargetProjectCxxCompileOptions,
		TargetProjectCxxDefines,
		TargetProjectCxxIncludeDirs,
		TargetProjectCxxLibDirs,
		TargetProjectCxxLinkerScript,
		TargetProjectCxxLinkerOptions,
		TargetProjectCxxLinks,
		TargetProjectCxxMacOsFrameworkPaths,
		TargetProjectCxxMacOsFrameworks,
		TargetProjectCxxObjectiveCxx,
		TargetProjectCxxPrecompiledHeader,
		TargetProjectCxxThreads,
		TargetProjectCxxRunTimeTypeInfo,
		TargetProjectCxxExceptions,
		TargetProjectCxxStaticLinking,
		TargetProjectCxxStaticLinks,
		TargetProjectCxxWarnings,
		TargetProjectCxxWindowsAppManifest,
		TargetProjectCxxWindowsAppIcon,
		TargetProjectCxxWindowsOutputDef,
		TargetProjectCxxWindowsPrefixOutputFilename,
		//
		TargetScript,
		TargetScriptScript,
		//
		TargetCMake,
		TargetCMakeLocation,
		TargetCMakeBuildFile,
		TargetCMakeDefines,
		TargetCMakeRecheck,
		TargetCMakeToolset,
		//
		TargetChalet,
		TargetChaletLocation,
		TargetChaletBuildFile,
		TargetChaletRecheck,
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
