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
struct SourceOutputs;
struct CommandLineInputs;

struct AssemblyDumper
{
	explicit AssemblyDumper(const CommandLineInputs& inInputs, BuildState& inState);

	bool validate() const;

	bool dumpProject(const std::string& inProjectName, const SourceOutputs& inOutputs, const bool inForced = false) const;

private:
	CommandPool::CmdList getAsmCommands(const SourceOutputs& inOutputs, const bool inForced) const;
	StringList getAsmGenerate(const std::string& object, const std::string& target) const;

	const CommandLineInputs& m_inputs;
	BuildState& m_state;

	CommandPool m_commandPool;
	mutable StringList m_cache;
};
}

#endif // CHALET_ASSEMBLY_DUMPER_HPP
