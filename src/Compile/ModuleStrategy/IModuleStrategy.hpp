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
struct IModuleStrategy;
using ModuleStrategy = Unique<IModuleStrategy>;

struct IModuleStrategy
{
	explicit IModuleStrategy(BuildState& inState);
	CHALET_DISALLOW_COPY_MOVE(IModuleStrategy);
	virtual ~IModuleStrategy() = default;

	[[nodiscard]] static ModuleStrategy make(const ToolchainType inType, BuildState& inState);

	virtual bool buildProject(const SourceTarget& inProject, SourceOutputs&& inOutputs, CompileToolchain& inToolchain);

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

	CommandPool::CmdList getModuleCommands(CompileToolchainController& inToolchain, const SourceFileGroupList& inGroups, const Dictionary<ModulePayload>& inModules, const ModuleFileType inType);
	void addCompileCommands(CommandPool::CmdList& outList, CompileToolchainController& inToolchain, const SourceFileGroupList& inGroups);
	CommandPool::Cmd getLinkCommand(CompileToolchainController& inToolchain, const SourceTarget& inProject, const std::string& inTarget, const StringList& inLinks);

	BuildState& m_state;

	std::string m_rootModule;
	std::string m_msvcToolsDirectory;
	Dictionary<bool> m_compileCache;

	StrategyType m_oldStrategy = StrategyType::None;

	bool m_sourcesChanged = false;
	bool m_generateDependencies = false;
};
}

#endif // CHALET_IMODULE_STRATEGY_HPP
