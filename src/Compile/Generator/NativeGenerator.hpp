/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include "Compile/CommandPool.hpp"
#include "Compile/CompileToolchainController.hpp"
#include "Compile/NativeCompileAdapter.hpp"
#include "State/SourceFileGroup.hpp"
#include "State/Target/SourceTarget.hpp"

namespace chalet
{
class BuildState;
struct SourceOutputs;

class NativeGenerator
{
public:
	NativeGenerator(BuildState& inState);

	bool addProject(const SourceTarget& inProject, const Unique<SourceOutputs>& inOutputs, CompileToolchain& inToolchain);

	bool buildProject(const SourceTarget& inProject);

	void initialize();
	void dispose() const;

	bool targetCompiled() const noexcept;

private:
	CommandPool::CmdList getPchCommands(const std::string& pchTarget);
	CommandPool::CmdList getCompileCommands(const SourceFileGroupList& inGroups);

	StringList getCxxCompile(const std::string& source, const std::string& target, const SourceType derivative) const;
	StringList getRcCompile(const std::string& source, const std::string& target) const;

	void checkCommandsForChanges();

	BuildState& m_state;

	NativeCompileAdapter m_compileAdapter;

	mutable Unique<CommandPool> m_commandPool;

	Dictionary<CommandPool::JobList> m_targets;

	const SourceTarget* m_project = nullptr;
	CompileToolchainController* m_toolchain = nullptr;

	std::unordered_set<std::string> m_fileCache;

	std::array<bool, static_cast<size_t>(SourceType::Count)> m_commandsChanged;
	bool m_targetCommandChanged = false;

	bool m_pchChanged = false;
	bool m_sourcesChanged = false;
	bool m_linkTarget = false;
};
}
