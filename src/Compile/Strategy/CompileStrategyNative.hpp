/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_COMPILE_STRATEGY_NATIVE_HPP
#define CHALET_COMPILE_STRATEGY_NATIVE_HPP

#include "Compile/Strategy/ICompileStrategy.hpp"

#include "Compile/CommandPool.hpp"
#include "State/Target/SourceTarget.hpp"

namespace chalet
{
class BuildState;

class CompileStrategyNative final : public ICompileStrategy
{
public:
	explicit CompileStrategyNative(BuildState& inState);

	virtual bool initialize() final;
	virtual bool addProject(const SourceTarget& inProject) final;

	virtual bool doPreBuild() final;
	virtual bool doPostBuild() const final;
	virtual bool buildProject(const SourceTarget& inProject) final;

private:
	CommandPool::CmdList getPchCommands(const std::string& pchTarget);
	CommandPool::CmdList getCompileCommands(const SourceFileGroupList& inGroups);
	CommandPool::CmdList getLinkCommand(const std::string& inTarget, const StringList& inObjects);

	StringList getCxxCompile(const std::string& source, const std::string& target, const SourceType derivative) const;
	StringList getRcCompile(const std::string& source, const std::string& target) const;

	StringList getCommandWithShell(StringList&& inCmd) const;

	mutable Unique<CommandPool> m_commandPool;

	StringList m_fileCache;

	const SourceTarget* m_project = nullptr;
	CompileToolchainController* m_toolchain = nullptr;

	Dictionary<CommandPool::JobList> m_targets;

	bool m_generateDependencies = false;
	bool m_pchChanged = false;
	bool m_sourcesChanged = false;
	bool m_runThroughShell = false;
	bool m_initialized = false;
};
}

#endif // CHALET_COMPILE_STRATEGY_NATIVE_HPP
