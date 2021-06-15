/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_ASSEMBLY_DUMPER_HPP
#define CHALET_ASSEMBLY_DUMPER_HPP

#include "Compile/CommandPool.hpp"
#include "State/Target/ProjectTarget.hpp"

namespace chalet
{
class BuildState;
struct ProjectTarget;

struct AssemblyDumper
{
	explicit AssemblyDumper(BuildState& inState);

	bool addProject(const ProjectTarget& inProject, StringList&& inAssemblies);

	bool dumpProject(const ProjectTarget& inProject) const;

private:
	CommandPool::CmdList getAsmCommands(const StringList& inAssemblies) const;
	StringList getAsmGenerate(const std::string& object, const std::string& target) const;

	BuildState& m_state;
	const ProjectTarget* m_project = nullptr;

	std::unordered_map<std::string, StringList> m_outputs;

	CommandPool m_commandPool;
	mutable StringList m_cache;
};
}

#endif // CHALET_ASSEMBLY_DUMPER_HPP
