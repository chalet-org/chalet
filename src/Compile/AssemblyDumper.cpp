/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/AssemblyDumper.hpp"

#include "Cache/SourceCache.hpp"
#include "Core/CommandLineInputs.hpp"
#include "State/AncillaryTools.hpp"
#include "State/BuildState.hpp"
#include "State/SourceOutputs.hpp"
#include "State/Target/ProjectTarget.hpp"
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
bool AssemblyDumper::dumpProject(const std::string& inProjectName, const SourceOutputs& inOutputs, const bool inForced) const
{
	CommandPool::Target target;
	target.list = getAsmCommands(inOutputs, inForced);

	CommandPool::Settings settings;
	settings.msvcCommand = false;
	settings.showCommands = Output::showCommands();
	settings.quiet = Output::quietNonBuild();
	settings.renameAfterCommand = false;

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
CommandPool::CmdList AssemblyDumper::getAsmCommands(const SourceOutputs& inOutputs, const bool inForced) const
{
	CommandPool::CmdList ret;

	auto& sourceCache = m_state.cache.file().sources();

	for (auto& group : inOutputs.groups)
	{
		const auto& asmFile = group->assemblyFile;

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
			out.color = Output::theme().alt;
			out.symbol = " ";
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
			auto asmCommand = fmt::format("{disassembler} -tvV {object} | c++filt > {target}",
				FMT_ARG(disassembler),
				FMT_ARG(object),
				FMT_ARG(target));
			ret.emplace_back(std::move(asmCommand));
		}
		else
#endif
		{
			// objdump
			std::string archArg;
			if (!m_state.toolchain.isDisassemblerLLVMObjDump())
			{
				archArg = "-Mintel ";
			}
			auto asmCommand = fmt::format("\"{disassembler}\" -d -C {archArg}{object} > {target}",
				FMT_ARG(disassembler),
				FMT_ARG(archArg),
				FMT_ARG(object),
				FMT_ARG(target));
			ret.emplace_back(std::move(asmCommand));
		}
	}

	return ret;
}
}
