/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/AssemblyDumper.hpp"

#include "State/AncillaryTools.hpp"
#include "State/BuildState.hpp"
#include "State/Target/ProjectTarget.hpp"
#include "Terminal/Commands.hpp"
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
}

/*****************************************************************************/
bool AssemblyDumper::addProject(const ProjectTarget& inProject, StringList&& inAssemblies)
{
#if defined(CHALET_MACOS)
	if (m_state.tools.otool().empty())
	{
		Diagnostic::error("dumpAssembly feature requires otool, which is blank in the toolchain settings.");
		return false;
	}
#else
	if (m_state.toolchain.objdump().empty())
	{
		Diagnostic::error("dumpAssembly feature requires objdump, which is blank in the toolchain settings.");
		return false;
	}
#endif

	auto& name = inProject.name();
	m_outputs[name] = std::move(inAssemblies);

	return true;
}

/*****************************************************************************/
bool AssemblyDumper::dumpProject(const ProjectTarget& inProject, const bool inForced) const
{
	auto& name = inProject.name();
	if (m_outputs.find(name) == m_outputs.end())
		return true;

	auto& assemblies = m_outputs.at(name);

	CommandPool::Target target;
	target.list = getAsmCommands(assemblies, inForced);

	CommandPool::Settings settings;
	settings.msvcCommand = false;
	settings.showCommands = Output::showCommands();
	settings.quiet = Output::quietNonBuild();
	settings.renameAfterCommand = false;

	if (!target.list.empty())
	{
		if (!m_commandPool.run(target, settings))
		{
			Diagnostic::error("There was a problem dumping asm files for: {}", inProject.name());
			return false;
		}

		Output::lineBreak();
	}

	return true;
}

/*****************************************************************************/
CommandPool::CmdList AssemblyDumper::getAsmCommands(const StringList& inAssemblies, const bool inForced) const
{
	CommandPool::CmdList ret;

	const auto& objDir = m_state.paths.objDir();
	const auto& asmDir = m_state.paths.asmDir();

	auto& sourceCache = m_state.cache.file().sources();

	for (auto& asmFile : inAssemblies)
	{
		if (asmFile.empty() || List::contains(m_cache, asmFile))
			continue;

		if (inForced)
		{
			Commands::remove(asmFile);
		}

		std::string object = asmFile;
		String::replaceAll(object, asmDir, objDir);

		if (String::endsWith(".asm", object))
			object = object.substr(0, object.size() - 4);

		std::string source = object;
		String::replaceAll(source, objDir + '/', "");

		if (String::endsWith(".o", source))
			source = source.substr(0, source.size() - 2);
		else if (String::endsWith({ ".res", ".obj" }, source))
			source = source.substr(0, source.size() - 4);

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

	if (!m_state.tools.bash().empty() && m_state.tools.bashAvailable())
	{
#if defined(CHALET_MACOS)
		auto asmCommand = fmt::format("{otool} -tvV {object} | c++filt > {target}",
			fmt::arg("otool", m_state.tools.otool()),
			FMT_ARG(object),
			FMT_ARG(target));
#else
		const auto& objdump = m_state.toolchain.objdump();
		std::string archArg;
		if (!String::endsWith("llvm-objdump.exe", objdump))
		{
			archArg = "-Mintel ";
		}
		auto asmCommand = fmt::format("\"{objdump}\" -d -C {archArg}{object} > {target}",
			FMT_ARG(objdump),
			FMT_ARG(archArg),
			FMT_ARG(object),
			FMT_ARG(target));
#endif

		ret.push_back(m_state.tools.bash());
		ret.emplace_back("-c");
		ret.emplace_back(std::move(asmCommand));
	}

	return ret;
}
}
