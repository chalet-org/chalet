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

	virtual bool initialize() = 0;
	virtual bool isSystemModuleFile(const std::string& inFile) const = 0;
	virtual bool readModuleDependencies(const SourceOutputs& inOutputs, Dictionary<ModuleLookup>& outModules) = 0;
	virtual bool readIncludesFromDependencyFile(const std::string& inFile, StringList& outList) = 0;
	virtual bool scanSourcesForModuleDependencies(CommandPool::Job& outJob, CompileToolchainController& inToolchain, const SourceFileGroupList& inGroups) = 0;
	virtual bool scanHeaderUnitsForModuleDependencies(CommandPool::Job& outJob, CompileToolchainController& inToolchain, Dictionary<ModulePayload>& outPayload, const SourceFileGroupList& inGroups) = 0;

protected:
	virtual std::string getBuildOutputForFile(const SourceFileGroup& inSource, const bool inIsObject) const = 0;

	CommandPool::CmdList getModuleCommands(CompileToolchainController& inToolchain, const SourceFileGroupList& inGroups, const Dictionary<ModulePayload>& inModules, const ModuleFileType inType);

	void addCompileCommands(CommandPool::CmdList& outList, CompileToolchainController& inToolchain, const SourceFileGroupList& inGroups);
	CommandPool::CmdList getLinkCommand(CompileToolchainController& inToolchain, const std::string& inTarget, const StringList& inLinks);

	bool addModuleRecursively(ModuleLookup& outModule, const ModuleLookup& inModule, const Dictionary<ModuleLookup>& inModules, Dictionary<ModulePayload>& outPayload);

	void checkIncludedHeaderFilesForChanges(const SourceFileGroupList& inGroups);
	void checkForDependencyChanges(DependencyGraph& dependencyGraph) const;
	bool addSourceGroup(SourceFileGroup* inGroup, SourceFileGroupList& outList) const;
	bool makeModuleBatch(CompileToolchainController& inToolchain, const Dictionary<ModulePayload>& inModules, const SourceFileGroupList& inList, CommandPool::JobList& outJobList);
	std::vector<SourceFileGroup*> getSourceFileGroupsForBuild(DependencyGraph& outDependencyGraph, SourceFileGroupList& outList) const;
	void addModuleBuildJobs(CompileToolchainController& inToolchain, const Dictionary<ModulePayload>& inModules, SourceFileGroupList& sourceCompiles, DependencyGraph& outDependencyGraph, CommandPool::JobList& outJobList);
	void logPayload(const Dictionary<ModulePayload>& inPayload) const;
	void addToCompileCommandsJson(const std::string& inReference, StringList&& inCmd) const;
	CommandPool::Settings getCommandPoolSettings() const;

	BuildState& m_state;
	CompileCommandsGenerator& m_compileCommandsGenerator;

	std::string m_previousSource;
	std::string m_moduleId;

	StringList m_implementationUnits;
	mutable Dictionary<bool> m_compileCache;

	StrategyType m_oldStrategy = StrategyType::None;

	mutable bool m_sourcesChanged = false;

private:
	std::string getModuleId() const;
	bool rebuildRequiredFromLinks() const;

	bool onFailure();

	bool checkDependentTargets(const SourceTarget& inProject) const;
	bool anyCmakeOrSubChaletTargetsChanged() const;
	void checkCommandsForChanges(CompileToolchainController& inToolchain);

	StringList m_targetsChanged;

	Dictionary<std::string> m_systemModules;

	const SourceTarget* m_project = nullptr;

	bool m_moduleCommandsChanged = false;
	bool m_winResourceCommandsChanged = false;
	bool m_targetCommandChanged = false;
};
}
