/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/AssemblyDumper.hpp"

#include "State/AncillaryTools.hpp"
#include "State/BuildState.hpp"
#include "State/Target/ProjectTarget.hpp"
#include "State/WorkspaceEnvironment.hpp"
#include "Terminal/Output.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
AssemblyDumper::AssemblyDumper(BuildState& inState) :
	m_state(inState),
	m_commandPool(m_state.environment.maxJobs())
{
}

/*****************************************************************************/
bool AssemblyDumper::addProject(const ProjectTarget& inProject, StringList&& inAssemblies)
{
	auto& name = inProject.name();
	m_outputs[name] = std::move(inAssemblies);

	return true;
}

/*****************************************************************************/
bool AssemblyDumper::dumpProject(const ProjectTarget& inProject) const
{
	auto& name = inProject.name();
	if (m_outputs.find(name) == m_outputs.end())
		return true;

	auto& assemblies = m_outputs.at(name);

	CommandPool::Target target;
	target.list = getAsmCommands(assemblies);

	CommandPool::Settings settings;
	settings.msvcCommand = false;
	settings.cleanOutput = !m_state.environment.showCommands();
	settings.quiet = Output::quietNonBuild();
	settings.renameAfterCommand = false;

	if (!target.list.empty())
	{
		if (!m_commandPool.run(target, settings))
		{
			Diagnostic::error(fmt::format("There was a problem dumping asm files for: {}", inProject.name()));
			return false;
		}
	}

	return true;
}

/*****************************************************************************/
CommandPool::CmdList AssemblyDumper::getAsmCommands(const StringList& inAssemblies) const
{
	CommandPool::CmdList ret;

	const auto& objDir = m_state.paths.objDir();
	const auto& asmDir = m_state.paths.asmDir();

	for (auto& asmFile : inAssemblies)
	{
		if (asmFile.empty() || List::contains(m_cache, asmFile))
			continue;

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

		if (m_state.sourceCache.fileChangedOrDoesNotExist(source))
		{
			CommandPool::Cmd out;
			out.output = asmFile;
			out.command = getAsmGenerate(object, asmFile);
			out.color = Color::Magenta;
			out.symbol = " ";
			ret.push_back(std::move(out));
		}

		m_cache.push_back(asmFile);
	}

	return ret;
}

/*****************************************************************************/
StringList AssemblyDumper::getAsmGenerate(const std::string& object, const std::string& target) const
{
	StringList ret;

	if (!m_state.ancillaryTools.bash().empty() && m_state.ancillaryTools.bashAvailable())
	{
#if defined(CHALET_MACOS)
		auto asmCommand = fmt::format("{otool} -tvV {object} | c++filt > {target}",
			fmt::arg("otool", m_state.ancillaryTools.otool()),
			FMT_ARG(object),
			FMT_ARG(target));
#else
		auto asmCommand = fmt::format("{objdump} -d -C -Mintel {object} > {target}",
			fmt::arg("objdump", m_state.toolchain.objdump()),
			FMT_ARG(object),
			FMT_ARG(target));
#endif

		ret.push_back(m_state.ancillaryTools.bash());
		ret.push_back("-c");
		ret.push_back(std::move(asmCommand));
	}

	return ret;
}
}
