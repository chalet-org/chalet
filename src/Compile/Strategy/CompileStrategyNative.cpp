/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/Strategy/CompileStrategyNative.hpp"

#include "Cache/SourceCache.hpp"
#include "Cache/WorkspaceCache.hpp"
#include "Compile/Environment/ICompileEnvironment.hpp"
#include "Core/CommandLineInputs.hpp"
#include "Process/ProcessController.hpp"
#include "State/AncillaryTools.hpp"
#include "State/BuildInfo.hpp"
#include "State/BuildPaths.hpp"
#include "State/BuildState.hpp"
#include "State/CompilerTools.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Environment.hpp"
#include "Terminal/Output.hpp"
#include "Terminal/Unicode.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"

// TODO: Finish.

namespace chalet
{
/*****************************************************************************/
CompileStrategyNative::CompileStrategyNative(BuildState& inState) :
	ICompileStrategy(StrategyType::Native, inState)
{
	m_runThroughShell = m_state.environment->isEmscripten();
}

/*****************************************************************************/
bool CompileStrategyNative::initialize()
{
	if (m_initialized)
		return false;

	const auto& cachePathId = m_state.cachePathId();

	auto& cacheFile = m_state.cache.file();
	UNUSED(m_state.cache.getCachePath(cachePathId, CacheType::Local));

	const bool buildStrategyChanged = cacheFile.buildStrategyChanged();
	if (buildStrategyChanged)
	{
		Commands::removeRecursively(m_state.paths.buildOutputDir());
	}

	m_initialized = true;

	return true;
}

/*****************************************************************************/
bool CompileStrategyNative::addProject(const SourceTarget& inProject)
{
	const auto& name = inProject.name();

	m_project = &inProject;
	m_toolchain = m_toolchains.at(name).get();

	m_pchChanged = false;
	m_sourcesChanged = false;

	chalet_assert(m_project != nullptr, "");

	const auto pchTarget = m_state.paths.getPrecompiledHeaderTarget(*m_project);
	const auto& outputs = m_outputs.at(name);

	m_generateDependencies = !Environment::isContinuousIntegrationServer() && !m_state.environment->isMsvc();
	bool targetExists = Commands::pathExists(outputs->target);

	{
		CommandPool::JobList jobs;

		{
			auto target = std::make_unique<CommandPool::Job>();
			target->list = getPchCommands(pchTarget);
			if (!target->list.empty() || !targetExists)
			{
				jobs.emplace_back(std::move(target));
			}
		}

		bool compileTarget = m_sourcesChanged || m_pchChanged || !targetExists;
		{
			auto target = std::make_unique<CommandPool::Job>();
			target->list = getCompileCommands(outputs->groups);
			if (!target->list.empty() || !targetExists)
			{
				jobs.emplace_back(std::move(target));
				compileTarget = true;
			}
		}

		if (compileTarget && !List::contains(m_fileCache, outputs->target))
		{
			m_fileCache.push_back(outputs->target);

			auto target = std::make_unique<CommandPool::Job>();
			target->list = getLinkCommand(outputs->target, outputs->objectListLinker);
			if (!target->list.empty())
			{
				jobs.emplace_back(std::move(target));
			}
		}

		if (!jobs.empty())
		{
			if (m_targets.find(name) == m_targets.end())
			{
				m_targets.emplace(name, std::move(jobs));
			}
		}
	}

	m_toolchain = nullptr;
	m_project = nullptr;

	return ICompileStrategy::addProject(inProject);
}

/*****************************************************************************/
bool CompileStrategyNative::doPreBuild()
{
	m_commandPool = std::make_unique<CommandPool>(m_state.info.maxJobs());

	return ICompileStrategy::doPreBuild();
}

/*****************************************************************************/
bool CompileStrategyNative::doPostBuild() const
{
	m_commandPool.reset();

	return ICompileStrategy::doPostBuild();
}

/*****************************************************************************/
bool CompileStrategyNative::buildProject(const SourceTarget& inProject)
{
	if (m_targets.find(inProject.name()) == m_targets.end())
		return true;

	auto& buildJobs = m_targets.at(inProject.name());

	if (!buildJobs.empty())
	{
		CommandPool::Settings settings;
		settings.color = Output::theme().build;
		settings.msvcCommand = m_state.environment->isMsvc();
		settings.keepGoing = m_state.info.keepGoing();
		settings.showCommands = Output::showCommands();
		settings.quiet = Output::quietNonBuild();

		if (!m_commandPool->runAll(m_targets.at(inProject.name()), settings))
		{
			m_state.cache.file().setDisallowSave(true);

			Output::lineBreak();
			return false;
		}

		Output::lineBreak();
	}

	return true;
}

/*****************************************************************************/
CommandPool::CmdList CompileStrategyNative::getPchCommands(const std::string& pchTarget)
{
	chalet_assert(m_project != nullptr, "");

	CommandPool::CmdList ret;
	if (m_project->usesPrecompiledHeader())
	{
		auto& sourceCache = m_state.cache.file().sources();
		const auto& source = m_project->precompiledHeader();
		const auto& depDir = m_state.paths.depDir();
		const auto& objDir = m_state.paths.objDir();

#if defined(CHALET_MACOS)
		if (m_state.info.targetArchitecture() == Arch::Cpu::UniversalMacOS)
		{
			auto baseFolder = String::getPathFolder(pchTarget);
			auto filename = String::getPathFilename(pchTarget);

			for (auto& arch : m_state.inputs.universalArches())
			{
				auto outObject = fmt::format("{}_{}/{}", baseFolder, arch, filename);
				auto intermediateSource = String::getPathFolderBaseName(outObject);

				bool pchChanged = sourceCache.fileChangedOrDoesNotExist(source, outObject);
				m_pchChanged |= pchChanged;
				if (pchChanged)
				{
					auto pchCache = fmt::format("{}/{}", objDir, intermediateSource);
					if (!List::contains(m_fileCache, intermediateSource))
					{
						m_fileCache.emplace_back(std::move(pchCache));

						const auto dependency = fmt::format("{}/{}.d", depDir, source);

						CommandPool::Cmd out;
						out.output = fmt::format("{} ({})", source, arch);
						out.command = m_toolchain->compilerCxx->getPrecompiledHeaderCommand(source, outObject, m_generateDependencies, dependency, arch);
						if (m_runThroughShell)
							out.command = getCommandWithShell(std::move(out.command));

						ret.emplace_back(std::move(out));
					}
				}
			}
		}
		else
#endif
		{
			bool pchChanged = sourceCache.fileChangedOrDoesNotExist(source, pchTarget);
			m_pchChanged |= pchChanged;
			if (pchChanged)
			{
				auto pchCache = fmt::format("{}/{}", objDir, source);
				if (!List::contains(m_fileCache, pchCache))
				{
					m_fileCache.emplace_back(std::move(pchCache));

					const auto dependency = fmt::format("{}/{}.d", depDir, source);

					CommandPool::Cmd out;
					out.output = source;
					out.command = m_toolchain->compilerCxx->getPrecompiledHeaderCommand(source, pchTarget, m_generateDependencies, dependency, std::string());
					if (m_runThroughShell)
						out.command = getCommandWithShell(std::move(out.command));

					const auto& cxxExt = m_state.paths.cxxExtension();
					if (!cxxExt.empty())
						out.reference = fmt::format("{}.{}", out.output, cxxExt);

					ret.emplace_back(std::move(out));
				}
			}
		}

		m_sourcesChanged |= m_pchChanged;
	}

	return ret;
}

/*****************************************************************************/
CommandPool::CmdList CompileStrategyNative::getCompileCommands(const SourceFileGroupList& inGroups)
{
	chalet_assert(m_project != nullptr, "");

	auto& sourceCache = m_state.cache.file().sources();

	CommandPool::CmdList ret;

	const auto& objDir = m_state.paths.objDir();

	const bool objectiveCxx = m_project->objectiveCxx();

	for (auto& group : inGroups)
	{
		const auto& source = group->sourceFile;
		const auto& target = group->objectFile;

		if (source.empty())
			continue;

		if (!objectiveCxx && (group->type == SourceType::ObjectiveC || group->type == SourceType::ObjectiveCPlusPlus))
			continue;

		switch (group->type)
		{
			case SourceType::WindowsResource: {
				bool sourceChanged = sourceCache.fileChangedOrDoesNotExist(source, target);
				m_sourcesChanged |= sourceChanged;
				if (sourceChanged || m_pchChanged)
				{
					auto sourceFile = fmt::format("{}/{}", objDir, source);
					if (!List::contains(m_fileCache, sourceFile))
					{
						m_fileCache.emplace_back(std::move(sourceFile));

						CommandPool::Cmd out;
						out.output = std::move(source);
						out.command = getRcCompile(source, target);
						out.reference = out.output;

						ret.emplace_back(std::move(out));
					}
				}
				break;
			}
			case SourceType::C:
			case SourceType::CPlusPlus:
			case SourceType::ObjectiveC:
			case SourceType::ObjectiveCPlusPlus: {
				bool sourceChanged = sourceCache.fileChangedOrDoesNotExist(source, target);
				m_sourcesChanged |= sourceChanged;
				if (sourceChanged || m_pchChanged)
				{
					auto sourceFile = fmt::format("{}/{}", objDir, source);
					if (!List::contains(m_fileCache, sourceFile))
					{
						m_fileCache.emplace_back(std::move(sourceFile));

						CommandPool::Cmd out;
						out.output = std::move(source);
						out.command = getCxxCompile(source, target, group->type);
						out.reference = out.output;

						ret.emplace_back(std::move(out));
					}
				}
				break;
			}

			case SourceType::CxxPrecompiledHeader:
			case SourceType::Unknown:
			default:
				break;
		}
	}

	return ret;
}

/*****************************************************************************/
CommandPool::CmdList CompileStrategyNative::getLinkCommand(const std::string& inTarget, const StringList& inObjects)
{
	chalet_assert(m_project != nullptr, "");
	chalet_assert(m_toolchain != nullptr, "");

	const auto targetBasename = m_state.paths.getTargetBasename(*m_project);

	CommandPool::CmdList ret;

	{
		CommandPool::Cmd out;
		out.command = m_toolchain->getOutputTargetCommand(inTarget, inObjects, targetBasename);
		if (m_runThroughShell)
			out.command = getCommandWithShell(std::move(out.command));

		auto label = m_project->isStaticLibrary() ? "Archiving" : "Linking";
		out.output = fmt::format("{} {}", label, inTarget);

		ret.emplace_back(std::move(out));
	}

	return ret;
}

/*****************************************************************************/
StringList CompileStrategyNative::getCxxCompile(const std::string& source, const std::string& target, const SourceType derivative) const
{
	chalet_assert(m_toolchain != nullptr, "");

	StringList ret;

	const auto& depDir = m_state.paths.depDir();
	const auto dependency = fmt::format("{depDir}/{source}.d", FMT_ARG(depDir), FMT_ARG(source));

	ret = m_toolchain->compilerCxx->getCommand(source, target, m_generateDependencies, dependency, derivative);
	if (m_runThroughShell)
		ret = getCommandWithShell(std::move(ret));

	return ret;
}

/*****************************************************************************/
StringList CompileStrategyNative::getRcCompile(const std::string& source, const std::string& target) const
{
	chalet_assert(m_toolchain != nullptr, "");

	StringList ret;

	const auto& depDir = m_state.paths.depDir();
	const auto dependency = fmt::format("{depDir}/{source}.d", FMT_ARG(depDir), FMT_ARG(source));

	ret = m_toolchain->compilerWindowsResource->getCommand(source, target, m_generateDependencies, dependency);
	if (m_runThroughShell)
		ret = getCommandWithShell(std::move(ret));

	return ret;
}

/*****************************************************************************/
StringList CompileStrategyNative::getCommandWithShell(StringList&& inCmd) const
{
#if defined(CHALET_WIN32)
	// return List::combine(StringList{ m_state.tools.powershell() }, std::move(inCmd));
	return StringList{ m_state.tools.commandPrompt(), "/c", String::join(std::move(inCmd)) };
#else
	return std::move(inCmd);
#endif
}

}
