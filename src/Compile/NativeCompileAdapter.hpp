/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include "Compile/CommandPool.hpp"
#include "CompileToolchainController.hpp"

namespace chalet
{
class BuildState;
struct SourceCache;
struct SourceOutputs;
struct SourceTarget;

struct NativeCompileAdapter
{
	explicit NativeCompileAdapter(const BuildState& inState);

	void addChangedTarget(const SourceTarget& inProject);

	bool checkDependentTargets(const SourceTarget& inProject) const;
	bool rebuildRequiredFromLinks(const SourceTarget& inProject) const;
	bool anyCmakeOrSubChaletTargetsChanged() const;

	void setDependencyCacheSize(const size_t inSize);
	void clearDependencyCache();
	bool fileChangedOrDependentChanged(const std::string& source, const std::string& target, const std::string& dependency);
	bool anyDependenciesChanged(const std::string& dependency);

	CommandPool::Settings getCommandPoolSettings() const;
	CommandPool::CmdList getLinkCommandList(const SourceTarget& inProject, CompileToolchainController& inToolchain, const SourceOutputs& inOutputs) const;

private:
	CommandPool::Cmd getLinkCommand(const SourceTarget& inProject, CompileToolchainController& inToolchain, const SourceOutputs& inOutputs) const;

	const BuildState& m_state;
	SourceCache& m_sourceCache;

	StringList m_targetsChanged;

	std::unordered_set<std::string> m_dependencyCache;
};
}
