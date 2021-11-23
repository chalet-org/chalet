/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/AssemblyDumper.hpp"

#include "Cache/SourceCache.hpp"
#include "Cache/WorkspaceCache.hpp"
#include "Core/CommandLineInputs.hpp"
#include "State/AncillaryTools.hpp"
#include "State/BuildInfo.hpp"
#include "State/BuildState.hpp"
#include "State/CompilerTools.hpp"
#include "State/SourceOutputs.hpp"
#include "State/Target/SourceTarget.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Output.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
AssemblyDumper::AssemblyDumper(const CommandLineInputs& inInputs, BuildState& inState) :
	m_inputs(inInputs),
	m_state(inState),
	m_commandPool(m_state.info.maxJobs())
{
}

/*****************************************************************************/
bool AssemblyDumper::validate() const
{
	if (m_state.toolchain.disassembler().empty())
	{
#if defined(CHALET_WIN32)
		Diagnostic::error("{}: dumpAssembly feature requires dumpbin (if MSVC) or objdump (if MinGW), which is blank in the toolchain settings.", m_inputs.settingsFile());
#elif defined(CHALET_MACOS)
		Diagnostic::error("{}: dumpAssembly feature requires otool or objdump as the disassembler, which is blank in the toolchain settings.", m_inputs.settingsFile());
#else
		Diagnostic::error("{}: dumpAssembly feature requires objdump as the disassembler, which is blank in the toolchain settings.", m_inputs.settingsFile());
#endif
		return false;
	}

	if (
#if defined(CHALET_WIN32)
		!m_state.toolchain.isDisassemblerDumpBin() &&
#endif
		!m_state.tools.bashAvailable())
	{
#if defined(CHALET_MACOS)
		Diagnostic::error("{}: dumpAssembly feature for otool and objdump require bash, but what not detected or blank in tools.", m_inputs.settingsFile());
#else
		Diagnostic::error("{}: dumpAssembly feature for objdump require bash, but what not detected or blank in tools.", m_inputs.settingsFile());
#endif
		return false;
	}

	return true;
}

/*****************************************************************************/
bool AssemblyDumper::dumpProject(const std::string& inProjectName, Unique<SourceOutputs>&& inOutputs, const bool inForced)
{
	CommandPool::Target target;
	target.list = getAsmCommands(*inOutputs, inForced);

	CommandPool::Settings settings;
	settings.color = Output::theme().assembly;
	settings.msvcCommand = false;
	settings.showCommands = Output::showCommands();
	settings.quiet = Output::quietNonBuild();
	settings.renameAfterCommand = false;

	inOutputs.reset();

	if (!target.list.empty())
	{
		if (!m_commandPool.run(target, settings))
		{
			Diagnostic::error("There was a problem dumping asm files for: {}", inProjectName);
			return false;
		}

		Output::lineBreak();
	}

	return true;
}

/*****************************************************************************/
CommandPool::CmdList AssemblyDumper::getAsmCommands(const SourceOutputs& inOutputs, const bool inForced)
{
	CommandPool::CmdList ret;

	auto& sourceCache = m_state.cache.file().sources();

	for (auto& group : inOutputs.groups)
	{
		const auto& asmFile = group->otherFile;

		if (asmFile.empty() || List::contains(m_cache, asmFile))
			continue;

		if (inForced)
			Commands::remove(asmFile);

		const auto& source = group->sourceFile;
		const auto& object = group->objectFile;

		if (sourceCache.fileChangedOrDoesNotExist(source, asmFile))
		{
			CommandPool::Cmd out;
			out.output = asmFile;
			out.command = getAsmGenerate(object, asmFile);
			ret.emplace_back(std::move(out));
		}

		m_cache.push_back(asmFile);
	}

	return ret;
}

/*****************************************************************************/
StringList AssemblyDumper::getAsmGenerate(const std::string& object, const std::string& target) const
{
	StringList ret;

	const auto& disassembler = m_state.toolchain.disassembler();

#if defined(CHALET_WIN32)
	// dumpbin
	// https://docs.microsoft.com/en-us/cpp/build/reference/dumpbin-options?view=msvc-160
	if (m_state.toolchain.isDisassemblerDumpBin())
	{
		ret.push_back(disassembler);
		ret.emplace_back("/nologo");
		ret.emplace_back("/disasm");
		ret.emplace_back(fmt::format("/out:{}", target));
		ret.push_back(object);

		// TODO: /PDBPATH ?
	}
	else
#endif
		if (m_state.tools.bashAvailable())
	{
		ret.push_back(m_state.tools.bash());
		ret.emplace_back("-c");

#if defined(CHALET_MACOS)
		// otool
		if (m_state.toolchain.isDisassemblerOtool())
		{
			StringList cmd{ disassembler };
			cmd.emplace_back("-tvV");
			cmd.emplace_back(object);
			cmd.emplace_back("|");
			cmd.emplace_back("c++filt");
			cmd.emplace_back(">");
			cmd.emplace_back(target);
			ret.emplace_back(String::join(cmd));
		}
		else
#endif
		{
			// objdump
			StringList cmd{ fmt::format("\"{}\"", disassembler) };
			cmd.emplace_back("-d");
			cmd.emplace_back("-C");

#if defined(CHALET_WIN32) || defined(CHALET_LINUX)
			if (!m_state.toolchain.isDisassemblerLLVMObjDump())
			{
				Arch::Cpu arch = m_state.info.targetArchitecture();
	#if defined(CHALET_LINUX)
				if (arch == Arch::Cpu::X64)
				{
					cmd.emplace_back("-Mintel,x86-64");
				}
				else if (arch == Arch::Cpu::X86)
				{
					cmd.emplace_back("-Mintel,i686");
				}
	#else
				if (arch == Arch::Cpu::X64 || arch == Arch::Cpu::X86)
				{
					cmd.emplace_back("-Mintel");
				}
	#endif
			}
#endif
			cmd.emplace_back(object);
			cmd.emplace_back(">");
			cmd.emplace_back(target);
			ret.emplace_back(String::join(cmd));
		}
	}

	return ret;
}
}
