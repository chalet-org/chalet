/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_ASSEMBLY_DUMPER_HPP
#define CHALET_ASSEMBLY_DUMPER_HPP

#include "Compile/CommandPool.hpp"
#include "State/Target/SourceTarget.hpp"

namespace chalet
{
class BuildState;
struct SourceTarget;
struct SourceOutputs;

struct AssemblyDumper
{
	explicit AssemblyDumper(BuildState& inState);

	bool validate() const;

	bool dumpProject(const std::string& inProjectName, Unique<SourceOutputs>&& inOutputs, const bool inForced = false);

private:
	CommandPool::CmdList getAsmCommands(const SourceOutputs& inOutputs, const bool inForced);
	StringList getAsmGenerate(const std::string& object, const std::string& target) const;

	BuildState& m_state;

	CommandPool m_commandPool;
	StringList m_cache;

	bool m_isDisassemblerDumpBin = false;
	bool m_isDisassemblerOtool = false;
	bool m_isDisassemblerLLVMObjDump = false;
	bool m_isDisassemblerWasm2Wat = false;
};
}

#endif // CHALET_ASSEMBLY_DUMPER_HPP
