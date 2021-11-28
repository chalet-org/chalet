/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_IMODULE_STRATEGY_HPP
#define CHALET_IMODULE_STRATEGY_HPP

#include "Compile/CommandPool.hpp"
#include "Compile/CompileToolchainController.hpp"
#include "Compile/Strategy/StrategyType.hpp"
#include "Compile/ToolchainType.hpp"
#include "State/SourceOutputs.hpp"
#include "State/Target/SourceTarget.hpp"

namespace chalet
{
class BuildState;
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
	};

	struct ModulePayload
	{
		StringList moduleTranslations;
		StringList headerUnitTranslations;
	};
	using DependencyGraph = std::unordered_map<SourceFileGroup*, std::vector<SourceFileGroup*>>;

public:
	explicit IModuleStrategy(BuildState& inState);
	CHALET_DISALLOW_COPY_MOVE(IModuleStrategy);
	virtual ~IModuleStrategy() = default;

	[[nodiscard]] static ModuleStrategy make(const ToolchainType inType, BuildState& inState);

	virtual bool buildProject(const SourceTarget& inProject, Unique<SourceOutputs>&& inOutputs, CompileToolchain&& inToolchain);

	virtual bool initialize() = 0;
	virtual bool readModuleDependencies(const SourceOutputs& inOutputs, Dictionary<ModuleLookup>& outModules) = 0;

protected:
	virtual std::string getBuildOutputForFile(const SourceFileGroup& inSource, const bool inIsObject);

	CommandPool::CmdList getModuleCommands(CompileToolchainController& inToolchain, const SourceFileGroupList& inGroups, const Dictionary<ModulePayload>& inModules, const ModuleFileType inType);
	void addCompileCommands(CommandPool::CmdList& outList, CompileToolchainController& inToolchain, const SourceFileGroupList& inGroups);
	CommandPool::CmdList getLinkCommand(CompileToolchainController& inToolchain, const SourceTarget& inProject, const std::string& inTarget, const StringList& inLinks);

	bool addHeaderUnitsRecursively(ModuleLookup& outModule, const ModuleLookup& inModule, const Dictionary<ModuleLookup>& inModules, Dictionary<ModulePayload>& outPayload);

	BuildState& m_state;

	const std::string kRootModule;

	std::string m_previousSource;

	std::string m_rootModule;
	Dictionary<bool> m_compileCache;

	StrategyType m_oldStrategy = StrategyType::None;

	bool m_sourcesChanged = false;
	bool m_generateDependencies = false;

private:
	std::string getModuleId() const;
};
}

#endif // CHALET_IMODULE_STRATEGY_HPP
