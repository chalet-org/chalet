/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/Strategy/CompileStrategyNative.hpp"

#include "State/AncillaryTools.hpp"
#include "Terminal/Color.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Environment.hpp"
#include "Terminal/Output.hpp"
#include "Terminal/Unicode.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"
#include "Utility/Subprocess.hpp"

// TODO: Finish. It's a bit crap

namespace chalet
{
/*****************************************************************************/
CompileStrategyNative::CompileStrategyNative(BuildState& inState) :
	ICompileStrategy(StrategyType::Native, inState),
	m_commandPool(m_state.environment.maxJobs())
{
}

/*****************************************************************************/
bool CompileStrategyNative::initialize(const StringList& inFileExtensions)
{
	if (m_initialized)
		return false;

	auto id = fmt::format("native_{}_{}", Output::showCommands() ? 1 : 0, String::join(inFileExtensions));
	std::string cachePath = m_state.cache.getHashPath(id, CacheType::Local);

	auto& cacheFile = m_state.cache.file();
	const auto& oldStrategyHash = cacheFile.hashStrategy();

	const bool appVersionChanged = cacheFile.appVersionChanged();
	auto strategyHash = String::getPathFilename(cachePath);
	cacheFile.setSourceCache(strategyHash);

	bool cacheNeedsUpdate = oldStrategyHash != strategyHash || appVersionChanged;

	if (!Commands::pathExists(cachePath))
	{
		std::ofstream(cachePath) << "native compile stub"
								 << std::endl;
	}

	if (cacheNeedsUpdate)
	{
		m_state.cache.file().setHashStrategy(std::move(strategyHash));
	}

	m_initialized = true;

	return true;
}

/*****************************************************************************/
bool CompileStrategyNative::addProject(const ProjectTarget& inProject, SourceOutputs&& inOutputs, CompileToolchain& inToolchain)
{
	m_project = &inProject;
	m_toolchain = inToolchain.get();

	m_pchChanged = false;
	m_sourcesChanged = false;

	chalet_assert(m_project != nullptr, "");

	const auto& compilerConfig = m_state.toolchain.getConfig(m_project->language());
	const auto pchTarget = m_state.paths.getPrecompiledHeaderTarget(*m_project, compilerConfig.isClangOrMsvc());

	m_generateDependencies = !Environment::isContinuousIntegrationServer() && !compilerConfig.isMsvc();

	auto target = std::make_unique<CommandPool::Target>();
	target->pre = getPchCommand(pchTarget);
	target->list = getCompileCommands(inOutputs.objectList);

	if (!target->list.empty())
	{
		if (m_sourcesChanged || m_pchChanged)
		{
			if (!List::contains(m_fileCache, inOutputs.target))
			{
				m_fileCache.push_back(inOutputs.target);

				target->post = getLinkCommand(inOutputs.target, inOutputs.objectListLinker);
			}
		}

		const auto& name = inProject.name();

		if (m_targets.find(name) == m_targets.end())
		{
			m_targets.emplace(name, std::move(target));
		}
		m_outputs[name] = std::move(inOutputs);
	}

	m_toolchain = nullptr;
	m_project = nullptr;

	return true;
}

/*****************************************************************************/
bool CompileStrategyNative::saveBuildFile() const
{
	return true;
}

/*****************************************************************************/
bool CompileStrategyNative::buildProject(const ProjectTarget& inProject) const
{
	if (m_targets.find(inProject.name()) == m_targets.end())
		return true;

	auto& target = *m_targets.at(inProject.name());

	const auto& config = m_state.toolchain.getConfig(inProject.language());

	CommandPool::Settings settings;
	settings.msvcCommand = config.isMsvc();
	settings.showCommands = Output::showCommands();
	settings.quiet = Output::quietNonBuild();
	settings.renameAfterCommand = m_generateDependencies;

	return m_commandPool.run(target, settings);
}

/*****************************************************************************/
CommandPool::Cmd CompileStrategyNative::getPchCommand(const std::string& pchTarget)
{
	chalet_assert(m_project != nullptr, "");

	CommandPool::Cmd ret;
	if (m_project->usesPch())
	{
		std::string source = m_project->pch();

		auto& sourceCache = m_state.cache.file().sources();

		m_pchChanged = sourceCache.fileChangedOrDoesNotExist(source, pchTarget);
		m_sourcesChanged |= m_pchChanged;
		if (m_pchChanged)
		{
			if (!List::contains(m_fileCache, source))
			{
				m_fileCache.push_back(source);

				auto tmp = getPchCompile(source, pchTarget);
				ret.output = std::move(source);
				ret.command = std::move(tmp.command);
				ret.renameFrom = std::move(tmp.renameFrom);
				ret.renameTo = std::move(tmp.renameTo);
				ret.color = Output::theme().build;
				ret.symbol = " ";
			}
		}
	}

	return ret;
}

/*****************************************************************************/
CommandPool::CmdList CompileStrategyNative::getCompileCommands(const StringList& inObjects)
{
	const auto& objDir = fmt::format("{}/", m_state.paths.objDir());

	auto& sourceCache = m_state.cache.file().sources();

	CommandPool::CmdList ret;

	for (auto& target : inObjects)
	{
		if (target.empty())
			continue;

		std::string source = target;
		String::replaceAll(source, objDir, "");

		if (String::endsWith(".o", source))
			source = source.substr(0, source.size() - 2);
		else if (String::endsWith({ ".res", ".obj" }, source))
			source = source.substr(0, source.size() - 4);

		CxxSpecialization specialization = CxxSpecialization::CPlusPlus;
		if (String::endsWith({ ".m", ".M" }, source))
			specialization = CxxSpecialization::ObjectiveC;
		else if (String::endsWith(".mm", source))
			specialization = CxxSpecialization::ObjectiveCPlusPlus;

		if (String::endsWith({ ".rc", ".RC" }, source))
		{
#if defined(CHALET_WIN32)
			bool sourceChanged = sourceCache.fileChangedOrDoesNotExist(source, target);
			m_sourcesChanged |= sourceChanged;
			if (sourceChanged || m_pchChanged)
			{
				if (!List::contains(m_fileCache, source))
				{
					m_fileCache.push_back(source);

					auto tmp = getRcCompile(source, target);
					CommandPool::Cmd out;
					out.output = std::move(source);
					out.command = std::move(tmp.command);
					out.renameFrom = std::move(tmp.renameFrom);
					out.renameTo = std::move(tmp.renameTo);
					out.color = Output::theme().build;
					out.symbol = " ";
					ret.emplace_back(std::move(out));
				}
			}
#else
			continue;
#endif
		}
		else
		{
			bool sourceChanged = sourceCache.fileChangedOrDoesNotExist(source, target);
			m_sourcesChanged |= sourceChanged;
			if (sourceChanged || m_pchChanged)
			{
				if (!List::contains(m_fileCache, source))
				{
					m_fileCache.push_back(source);

					auto tmp = getCxxCompile(source, target, specialization);
					CommandPool::Cmd out;
					out.output = std::move(source);
					out.command = std::move(tmp.command);
					out.renameFrom = std::move(tmp.renameFrom);
					out.renameTo = std::move(tmp.renameTo);
					out.color = Output::theme().build;
					out.symbol = " ";
					ret.emplace_back(std::move(out));
				}
			}
		}
	}

	return ret;
}

/*****************************************************************************/
CommandPool::Cmd CompileStrategyNative::getLinkCommand(const std::string& inTarget, const StringList& inObjects)
{
	chalet_assert(m_project != nullptr, "");
	chalet_assert(m_toolchain != nullptr, "");

	const auto targetBasename = m_state.paths.getTargetBasename(*m_project);

	const std::string description = m_project->isStaticLibrary() ? "ARCHIVE" : "LINK";

	CommandPool::Cmd ret;
	ret.command = m_toolchain->getLinkerTargetCommand(inTarget, inObjects, targetBasename);
	ret.label = std::move(description);
	ret.output = inTarget;
	ret.color = Output::theme().build;
	ret.symbol = " ";
	// ret.symbol = Unicode::rightwardsTripleArrow();

	return ret;
}

/*****************************************************************************/
CompileStrategyNative::CmdTemp CompileStrategyNative::getPchCompile(const std::string& source, const std::string& target) const
{
	chalet_assert(m_project != nullptr, "");
	chalet_assert(m_toolchain != nullptr, "");

	CmdTemp ret;

	if (m_project->usesPch())
	{
		const auto& depDir = m_state.paths.depDir();

		ret.renameFrom = fmt::format("{depDir}/{source}.Td", FMT_ARG(depDir), FMT_ARG(source));
		ret.renameTo = fmt::format("{depDir}/{source}.d", FMT_ARG(depDir), FMT_ARG(source));

		ret.command = m_toolchain->getPchCompileCommand(source, target, m_generateDependencies, ret.renameFrom);
	}

	return ret;
}

/*****************************************************************************/
CompileStrategyNative::CmdTemp CompileStrategyNative::getCxxCompile(const std::string& source, const std::string& target, CxxSpecialization specialization) const
{
	chalet_assert(m_toolchain != nullptr, "");

	CmdTemp ret;

	const auto& depDir = m_state.paths.depDir();

	ret.renameFrom = fmt::format("{depDir}/{source}.Td", FMT_ARG(depDir), FMT_ARG(source));
	ret.renameTo = fmt::format("{depDir}/{source}.d", FMT_ARG(depDir), FMT_ARG(source));

	ret.command = m_toolchain->getCxxCompileCommand(source, target, m_generateDependencies, ret.renameFrom, specialization);

	return ret;
}

/*****************************************************************************/
CompileStrategyNative::CmdTemp CompileStrategyNative::getRcCompile(const std::string& source, const std::string& target) const
{
	chalet_assert(m_toolchain != nullptr, "");

	CmdTemp ret;

#if defined(CHALET_WIN32)
	const auto& depDir = m_state.paths.depDir();

	ret.renameFrom = fmt::format("{depDir}/{source}.Td", FMT_ARG(depDir), FMT_ARG(source));
	ret.renameTo = fmt::format("{depDir}/{source}.d", FMT_ARG(depDir), FMT_ARG(source));

	ret.command = m_toolchain->getRcCompileCommand(source, target, m_generateDependencies, ret.renameFrom);
#else
	UNUSED(source, target);
#endif

	return ret;
}

}
