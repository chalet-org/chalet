/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include "Compile/CommandPool.hpp"
#include "Compile/CompileToolchainController.hpp"
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

	virtual bool buildProject(const SourceTarget& inProject, Unique<SourceOutputs>&& inOutputs, CompileToolchain&& inToolchain);

	virtual bool initialize();
	bool isSystemHeaderFileOrModuleFile(const std::string& inFile) const;
	virtual bool readModuleDependencies(const SourceOutputs& inOutputs, Dictionary<ModuleLookup>& outModules) = 0;
	virtual bool readIncludesFromDependencyFile(const std::string& inFile, StringList& outList) = 0;
	virtual bool scanSourcesForModuleDependencies(CommandPool::Job& outJob, CompileToolchainController& inToolchain, const SourceFileGroupList& inGroups) = 0;
	virtual bool scanHeaderUnitsForModuleDependencies(CommandPool::Job& outJob, CompileToolchainController& inToolchain, Dictionary<ModulePayload>& outPayload, const SourceFileGroupList& inGroups) = 0;

protected:
	std::string getBuildOutputForFile(const SourceFileGroup& inFile, const bool inIsObject) const;

	CommandPool::CmdList getModuleCommands(CompileToolchainController& inToolchain, const SourceFileGroupList& inGroups, const Dictionary<ModulePayload>& inModules, const ModuleFileType inType);

	CommandPool::CmdList getLinkCommand(CompileToolchainController& inToolchain, const std::string& inTarget, const StringList& inLinks);

	bool addModuleRecursively(ModuleLookup& outModule, const ModuleLookup& inModule, const Dictionary<ModuleLookup>& inModules, Dictionary<ModulePayload>& outPayload);

	void checkIncludedHeaderFilesForChanges(const SourceFileGroupList& inGroups);
	void checkForDependencyChanges(DependencyGraph& dependencyGraph) const;
	bool addSourceGroup(SourceFileGroup* inGroup, SourceFileGroupList& outList) const;
	void logPayload(const Dictionary<ModulePayload>& inPayload) const;
	void addToCompileCommandsJson(const std::string& inReference, StringList&& inCmd) const;
	CommandPool::Settings getCommandPoolSettings() const;

	BuildState& m_state;
	CompileCommandsGenerator& m_compileCommandsGenerator;

	std::string m_previousSource;
	std::string m_moduleId;

	StringList m_systemHeaderDirectories;
	StringList m_implementationUnits;
	mutable Dictionary<bool> m_compileCache;

	StrategyType m_oldStrategy = StrategyType::None;

	mutable bool m_sourcesChanged = false;

	const SourceTarget* m_project = nullptr;

private:
	bool addSystemModules(Dictionary<ModuleLookup>& modules, Dictionary<ModulePayload>& modulePayload, Unique<SourceOutputs>& inOutputs);
	bool addAllHeaderUnits(Dictionary<ModuleLookup>& modules, Dictionary<ModulePayload>& modulePayload, StringList& headerUnitObjects, SourceFileGroupList& headerUnitList);
	void sortHeaderUnits(SourceFileGroupList& userHeaderUnits, StringList& headerUnitObjects, SourceFileGroupList& headerUnitList);
	void addHeaderUnitsToTargetLinks(Unique<SourceOutputs>& inOutputs, StringList&& headerUnitObjects);
	void addHeaderUnitsBuildJob(CommandPool::JobList& jobs, CompileToolchainController& inToolchain, const SourceFileGroupList& inHeaderUnitList, const Dictionary<ModulePayload>& modulePayload);
	void buildDependencyGraphAndAddModulesBuildJobs(CommandPool::JobList& jobs, CompileToolchainController& inToolchain, const Dictionary<ModuleLookup>& modules, const Dictionary<ModulePayload>& modulePayload, Unique<SourceOutputs>& inOutputs);
	void addModulesBuildJobs(CommandPool::JobList& jobs, CompileToolchainController& inToolchain, const Dictionary<ModulePayload>& inModules, SourceFileGroupList& sourceCompiles, DependencyGraph& outDependencyGraph);
	bool makeModuleBatch(CommandPool::JobList& jobs, CompileToolchainController& inToolchain, const Dictionary<ModulePayload>& inModules, const SourceFileGroupList& inList);
	std::vector<SourceFileGroup*> getSourceFileGroupsForBuild(DependencyGraph& outDependencyGraph, SourceFileGroupList& outList) const;
	void addOtherBuildJobsToLastJob(CommandPool::JobList& jobs, CompileToolchainController& inToolchain, Unique<SourceOutputs>& inOutputs);
	void addOtherBuildCommands(CommandPool::CmdList& outList, CompileToolchainController& inToolchain, const SourceFileGroupList& inGroups);

	std::string getModuleId() const;
	bool rebuildRequiredFromLinks() const;
	bool cachedValue(const std::string& inSource) const;
	void setCompilerCache(const std::string& inSource, const bool inValue) const;

	bool onFailure();

	bool checkDependentTargets(const SourceTarget& inProject) const;
	bool anyCmakeOrSubChaletTargetsChanged() const;
	void checkCommandsForChanges(CompileToolchainController& inToolchain);

	StringList m_targetsChanged;

	Dictionary<std::string> m_systemModules;

	bool m_moduleCommandsChanged = false;
	bool m_winResourceCommandsChanged = false;
	bool m_targetCommandChanged = false;
};
}
