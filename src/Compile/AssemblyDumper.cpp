/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/AssemblyDumper.hpp"

#include "BuildEnvironment/IBuildEnvironment.hpp"
#include "Cache/SourceCache.hpp"
#include "Cache/WorkspaceCache.hpp"
#include "Core/CommandLineInputs.hpp"
#include "Process/Environment.hpp"
#include "State/AncillaryTools.hpp"
#include "State/BuildInfo.hpp"
#include "State/BuildPaths.hpp"
#include "State/BuildState.hpp"
#include "State/CompilerTools.hpp"
#include "State/SourceOutputs.hpp"
#include "State/Target/SourceTarget.hpp"
#include "System/Files.hpp"
#include "Terminal/Output.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
AssemblyDumper::AssemblyDumper(BuildState& inState) :
	m_state(inState),
	m_commandPool(m_state.info.maxJobs())
{
	m_isDisassemblerDumpBin = m_state.toolchain.isDisassemblerDumpBin();
	m_isDisassemblerOtool = m_state.toolchain.isDisassemblerOtool();
	m_isDisassemblerLLVMObjDump = m_state.toolchain.isDisassemblerLLVMObjDump();
	m_isDisassemblerWasm2Wat = m_state.environment->isEmscripten() && m_state.toolchain.isDisassemblerWasm2Wat();
}

/*****************************************************************************/
bool AssemblyDumper::validate() const
{
	auto& settings = m_state.inputs.settingsFile();
	auto dumpText = "The assembly dump feature";
	if (m_state.toolchain.disassembler().empty())
	{
		if (m_state.environment->isEmscripten())
		{
			auto pathKey = Environment::getPathKey();
			auto message = fmt::format("{}: {} requires 'wasm2wat' in {}. It's part of the  WebAssembly Binary Toolkit", settings, dumpText, pathKey);
#if defined(CHALET_WIN32)
			Diagnostic::error("{}, found here: https://github.com/WebAssembly/wabt/releases", message);
#elif defined(CHALET_MACOS)
			Diagnostic::error("{}. It can be installed from brew via 'wabt' or found here: https://github.com/WebAssembly/wabt/releases", message);
#else
			Diagnostic::error("{}, It can be installed from your package manager (if available) or found here: https://github.com/WebAssembly/wabt/releases", message);
#endif
		}
		else
		{
#if defined(CHALET_WIN32)
			Diagnostic::error("{}: {} requires dumpbin (if MSVC) or objdump (if MinGW), which is blank in the toolchain settings.", settings, dumpText);
#elif defined(CHALET_MACOS)
			Diagnostic::error("{}: {} requires otool or objdump as the disassembler, which is blank in the toolchain settings.", settings, dumpText);
#else
			Diagnostic::error("{}: {} requires objdump as the disassembler, which is blank in the toolchain settings.", settings, dumpText);
#endif
		}
		return false;
	}

	if (
#if defined(CHALET_WIN32)
		!m_isDisassemblerDumpBin &&
#endif
		!m_state.tools.bashAvailable())
	{
#if defined(CHALET_MACOS)
		Diagnostic::error("{}: {} for otool and objdump require bash, but what not detected or blank in tools.", settings, dumpText);
#else
		Diagnostic::error("{}: {} for objdump require bash, but what not detected or blank in tools.", settings, dumpText);
#endif
		return false;
	}

	return true;
}

/*****************************************************************************/
bool AssemblyDumper::dumpProject(const SourceTarget& inTarget, StringList& outCache, const bool inForced)
{
	CommandPool::Job target;

	{
		auto outputs = m_state.paths.getOutputs(inTarget, outCache);
		target.list = getAsmCommands(*outputs, inForced);
	}

	CommandPool::Settings settings;
	settings.color = Output::theme().assembly;
	settings.msvcCommand = false;
	settings.showCommands = Output::showCommands();
	settings.quiet = Output::quietNonBuild();
	if (!target.list.empty())
	{
		if (!m_commandPool.run(target, settings))
		{
			Diagnostic::error("There was a problem dumping asm files for: {}", inTarget.name());
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
		if (group->type == SourceType::CxxPrecompiledHeader)
			continue;

		const auto& asmFile = group->otherFile;

		if (asmFile.empty() || List::contains(m_cache, asmFile))
			continue;

		if (inForced)
			Files::removeIfExists(asmFile);

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
	if (m_isDisassemblerWasm2Wat)
	{
		ret.push_back(disassembler);
		ret.push_back(object);
		ret.emplace_back("-o");
		ret.push_back(target);
		return ret;
	}

#if defined(CHALET_WIN32)
	// dumpbin
	// https://docs.microsoft.com/en-us/cpp/build/reference/dumpbin-options?view=msvc-160
	if (m_isDisassemblerDumpBin)
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
		if (m_isDisassemblerOtool)
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
			if (!m_isDisassemblerLLVMObjDump)
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
