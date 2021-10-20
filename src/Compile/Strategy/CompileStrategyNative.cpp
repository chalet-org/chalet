/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/Strategy/CompileStrategyNative.hpp"

#include "Cache/SourceCache.hpp"
#include "Process/Process.hpp"
#include "State/AncillaryTools.hpp"
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
	ICompileStrategy(StrategyType::Native, inState),
	m_commandPool(m_state.info.maxJobs())
{
}

/*****************************************************************************/
bool CompileStrategyNative::initialize(const StringList& inFileExtensions)
{
	if (m_initialized)
		return false;

	const auto uniqueId = m_state.getUniqueIdForState(inFileExtensions);
	std::string cachePath = m_state.cache.getCachePath(uniqueId, CacheType::Local);

	auto& cacheFile = m_state.cache.file();

	cacheFile.setSourceCache(uniqueId, true);

	m_initialized = true;

	return true;
}

/*****************************************************************************/
bool CompileStrategyNative::addProject(const SourceTarget& inProject, SourceOutputs&& inOutputs, CompileToolchain& inToolchain)
{
	m_project = &inProject;
	m_toolchain = inToolchain.get();

	m_pchChanged = false;
	m_sourcesChanged = false;

	chalet_assert(m_project != nullptr, "");

	const auto& compilerConfig = m_state.toolchain.getConfig(m_project->language());
	const auto pchTarget = m_state.paths.getPrecompiledHeaderTarget(*m_project, compilerConfig);
	const auto& name = inProject.name();

	m_generateDependencies = !Environment::isContinuousIntegrationServer() && !compilerConfig.isMsvc();

	auto target = std::make_unique<CommandPool::Target>();
	target->pre = getPchCommands(pchTarget);
	target->list = getCompileCommands(inOutputs.groups);
	bool targetExists = Commands::pathExists(inOutputs.target);

	if (!target->list.empty() || !targetExists)
	{
		if (m_sourcesChanged || m_pchChanged || !targetExists)
		{
			if (!List::contains(m_fileCache, inOutputs.target))
			{
				m_fileCache.push_back(inOutputs.target);

				target->post = getLinkCommand(inOutputs.target, inOutputs.objectListLinker);
			}
		}

		if (m_targets.find(name) == m_targets.end())
		{
			m_targets.emplace(name, std::move(target));
		}
	}

	m_outputs[name] = std::move(inOutputs);

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
bool CompileStrategyNative::doPostBuild() const
{
	/*if (m_generateDependencies)
	{
		auto& sources = m_state.cache.file().sources();
		for (auto& [target, outputs] : m_outputs)
		{
			for (auto& group : outputs.groups)
			{
				if (sources.fileChangedOrDoesNotExist(group->objectFile, group->dependencyFile))
				{
					// std::ofstream(group->dependencyFile) << fmt::format("{}: \\\n  {}\n", group->objectFile, group->sourceFile);
				}
			}
		}
	}*/

	return true;
}

/*****************************************************************************/
bool CompileStrategyNative::buildProject(const SourceTarget& inProject)
{
	if (m_targets.find(inProject.name()) == m_targets.end())
		return true;

	auto& target = *m_targets.at(inProject.name());

	const auto& config = m_state.toolchain.getConfig(inProject.language());

	CommandPool::Settings settings;
	settings.color = Output::theme().build;
	settings.msvcCommand = config.isMsvc();
	settings.showCommands = Output::showCommands();
	settings.quiet = Output::quietNonBuild();
	settings.renameAfterCommand = m_generateDependencies;

	return m_commandPool.run(target, settings);
}

/*****************************************************************************/
CommandPool::CmdList CompileStrategyNative::getPchCommands(const std::string& pchTarget)
{
	chalet_assert(m_project != nullptr, "");

	CommandPool::CmdList ret;
	if (m_project->usesPch())
	{
		auto& sourceCache = m_state.cache.file().sources();
		std::string source = m_project->pch();
		const auto& depDir = m_state.paths.depDir();
		const auto& objDir = m_state.paths.objDir();

#if defined(CHALET_MACOS)
		if (m_state.info.targetArchitecture() == Arch::Cpu::UniversalMacOS)
		{
			auto baseFolder = String::getPathFolder(pchTarget);
			auto filename = String::getPathFilename(pchTarget);

			for (auto& arch : m_state.info.universalArches())
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
						auto command = m_toolchain->getPchCompileCommand(source, outObject, m_generateDependencies, dependency, arch);

						CommandPool::Cmd out;
						out.output = fmt::format("{} ({})", source, arch);
						out.command = std::move(command);
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
					auto command = m_toolchain->getPchCompileCommand(source, pchTarget, m_generateDependencies, dependency, std::string());

					CommandPool::Cmd out;
					out.output = std::move(source);
					out.command = std::move(command);
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
#if defined(CHALET_WIN32)
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
						ret.emplace_back(std::move(out));
					}
				}
#else
				continue;
#endif
				break;
			}
			case SourceType::C:
			case SourceType::CPlusPlus:
			case SourceType::ObjectiveC:
			case SourceType::ObjectiveCPlusPlus: {
				CxxSpecialization specialization = CxxSpecialization::CPlusPlus;
				if (group->type == SourceType::ObjectiveC)
					specialization = CxxSpecialization::ObjectiveC;
				else if (group->type == SourceType::ObjectiveCPlusPlus)
					specialization = CxxSpecialization::ObjectiveCPlusPlus;

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
						out.command = getCxxCompile(source, target, specialization);
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
CommandPool::Cmd CompileStrategyNative::getLinkCommand(const std::string& inTarget, const StringList& inObjects)
{
	chalet_assert(m_project != nullptr, "");
	chalet_assert(m_toolchain != nullptr, "");

	const auto targetBasename = m_state.paths.getTargetBasename(*m_project);

	CommandPool::Cmd ret;
	ret.command = m_toolchain->getLinkerTargetCommand(inTarget, inObjects, targetBasename);
	ret.label = m_project->isStaticLibrary() ? "Archiving" : "Linking";
	ret.output = inTarget;
	// ret.symbol = Unicode::rightwardsTripleArrow();

	return ret;
}

/*****************************************************************************/
StringList CompileStrategyNative::getCxxCompile(const std::string& source, const std::string& target, CxxSpecialization specialization) const
{
	chalet_assert(m_toolchain != nullptr, "");

	StringList ret;

	const auto& depDir = m_state.paths.depDir();
	const auto dependency = fmt::format("{depDir}/{source}.d", FMT_ARG(depDir), FMT_ARG(source));

	ret = m_toolchain->getCxxCompileCommand(source, target, m_generateDependencies, dependency, specialization);

	return ret;
}

/*****************************************************************************/
StringList CompileStrategyNative::getRcCompile(const std::string& source, const std::string& target) const
{
	chalet_assert(m_toolchain != nullptr, "");

	StringList ret;

#if defined(CHALET_WIN32)
	const auto& depDir = m_state.paths.depDir();
	const auto dependency = fmt::format("{depDir}/{source}.d", FMT_ARG(depDir), FMT_ARG(source));

	ret = m_toolchain->getRcCompileCommand(source, target, m_generateDependencies, dependency);
#else
	UNUSED(source, target);
#endif

	return ret;
}

}
