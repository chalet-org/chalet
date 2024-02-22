/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include "Compile/CommandPool.hpp"
#include "Compile/CompileToolchainController.hpp"
#include "Compile/NativeCompileAdapter.hpp"
#include "Compile/Strategy/StrategyType.hpp"
#include "Compile/ToolchainType.hpp"
#include "State/SourceOutputs.hpp"
#include "State/Target/SourceTarget.hpp"

namespace chalet
{
class BuildState;
struct CompileCommandsGenerator;

class IModuleStrategy;
using ModuleStrategy = Unique<IModuleStrategy>;

class IModuleStrategy
{
protected:
	struct ModuleLookup
	{
		std::string source;
		StringList importedModules;
		StringList importedHeaderUnits;
		bool systemModule = false;
	};

	struct ModulePayload
	{
		StringList moduleTranslations;
		StringList headerUnitTranslations;
	};
	using DependencyGraph = std::unordered_map<SourceFileGroup*, std::vector<SourceFileGroup*>>;

public:
	explicit IModuleStrategy(BuildState& inState, CompileCommandsGenerator& inCompileCommandsGenerator);
	CHALET_DISALLOW_COPY_MOVE(IModuleStrategy);
	virtual ~IModuleStrategy() = default;

	[[nodiscard]] static ModuleStrategy make(const ToolchainType inType, BuildState& inState, CompileCommandsGenerator& inCompileCommandsGenerator);

	virtual bool buildProject(const SourceTarget& inProject);

	Unique<SourceOutputs> outputs;
	CompileToolchain toolchain;

protected:
	virtual bool initialize();

	bool isSystemHeaderFileOrModuleFile(const std::string& inFile) const;
	virtual bool readModuleDependencies() = 0;
	virtual bool readIncludesFromDependencyFile(const std::string& inFile, StringList& outList) = 0;
	virtual bool scanSourcesForModuleDependencies(CommandPool::Job& outJob) = 0;
	virtual bool scanHeaderUnitsForModuleDependencies(CommandPool::Job& outJob) = 0;

	std::string getBuildOutputForFile(const SourceFileGroup& inFile, const bool inIsObject) const;

	CommandPool::CmdList getModuleCommands(const SourceFileGroupList& inSourceList, const Dictionary<ModulePayload>& inPayload, const ModuleFileType inType);

	CommandPool::CmdList getLinkCommand(const std::string& inTarget, const StringList& inLinks);

	bool addModuleRecursively(ModuleLookup& outModule, const ModuleLookup& inModule);

	void checkIncludedHeaderFilesForChanges();
	void checkForDependencyChanges(DependencyGraph& dependencyGraph) const;
	bool addSourceGroup(SourceFileGroup* inGroup, SourceFileGroupList& outList) const;
	void logPayload() const;
	void addToCompileCommandsJson(const std::string& inReference, StringList&& inCmd) const;
	CommandPool::Settings getCommandPoolSettings() const;

	BuildState& m_state;
	CompileCommandsGenerator& m_compileCommandsGenerator;

	NativeCompileAdapter m_compileAdapter;

	Dictionary<ModuleLookup> m_modules;
	Dictionary<ModulePayload> m_modulePayload;
	StringList m_headerUnitObjects;
	SourceFileGroupList m_headerUnitList;

	std::string m_previousSource;
	std::string m_moduleId;

	StringList m_systemHeaderDirectories;
	StringList m_implementationUnits;
	mutable Dictionary<bool> m_compileCache;

	StrategyType m_oldStrategy = StrategyType::None;

	mutable bool m_sourcesChanged = false;

	const SourceTarget* m_project = nullptr;

private:
	bool addSystemModules();
	bool addAllHeaderUnits();
	void sortHeaderUnits(SourceFileGroupList& userHeaderUnits);
	void addHeaderUnitsToTargetLinks();
	void addHeaderUnitsBuildJob(CommandPool::JobList& jobs);
	void buildDependencyGraphAndAddModulesBuildJobs(CommandPool::JobList& jobs);
	void addModulesBuildJobs(CommandPool::JobList& jobs, SourceFileGroupList& sourceCompiles, DependencyGraph& outDependencyGraph);
	bool makeModuleBatch(CommandPool::JobList& jobs, const SourceFileGroupList& inList);
	std::vector<SourceFileGroup*> getSourceFileGroupsForBuild(DependencyGraph& outDependencyGraph, SourceFileGroupList& outList) const;
	void addOtherBuildJobsToLastJob(CommandPool::JobList& jobs);
	void addOtherBuildCommands(CommandPool::CmdList& outList);

	std::string getModuleId() const;
	bool rebuildRequiredFromLinks() const;
	bool cachedValue(const std::string& inSource) const;
	void setCompilerCache(const std::string& inSource, const bool inValue) const;

	bool onFailure();

	void checkCommandsForChanges();

	Dictionary<std::string> m_systemModules;

	bool m_moduleCommandsChanged = false;
	bool m_winResourceCommandsChanged = false;
	bool m_targetCommandChanged = false;
};
}
