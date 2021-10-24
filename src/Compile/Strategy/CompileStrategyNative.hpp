/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_COMPILE_STRATEGY_NATIVE_HPP
#define CHALET_COMPILE_STRATEGY_NATIVE_HPP

#include "Compile/Strategy/ICompileStrategy.hpp"
#include "Compile/Toolchain/ICompileToolchain.hpp"

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
	virtual bool addProject(const SourceTarget& inProject, SourceOutputs&& inOutputs, CompileToolchain& inToolchain) final;

	virtual bool saveBuildFile() const final;
	virtual bool doPostBuild() const final;
	virtual bool buildProject(const SourceTarget& inProject) final;

private:
	CommandPool::CmdList getPchCommands(const std::string& pchTarget);
	CommandPool::CmdList getCompileCommands(const SourceFileGroupList& inGroups);
	CommandPool::Cmd getLinkCommand(const std::string& inTarget, const StringList& inObjects);

	StringList getCxxCompile(const std::string& source, const std::string& target, CxxSpecialization specialization) const;
	StringList getRcCompile(const std::string& source, const std::string& target) const;

	CommandPool m_commandPool;

	StringList m_fileCache;

	const SourceTarget* m_project = nullptr;
	ICompileToolchain* m_toolchain = nullptr;

	HeapDictionary<CommandPool::Target> m_targets;

	bool m_generateDependencies = false;
	bool m_pchChanged = false;
	bool m_sourcesChanged = false;
	bool m_initialized = false;
};
}

#endif // CHALET_COMPILE_STRATEGY_NATIVE_HPP
