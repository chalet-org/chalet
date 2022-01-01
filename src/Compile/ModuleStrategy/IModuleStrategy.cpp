/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/ModuleStrategy/IModuleStrategy.hpp"

#include "Cache/SourceCache.hpp"
#include "Cache/WorkspaceCache.hpp"
#include "Compile/Environment/ICompileEnvironment.hpp"
#include "Compile/ModuleStrategy/ModuleStrategyMSVC.hpp"
#include "State/BuildInfo.hpp"
#include "State/BuildPaths.hpp"
#include "State/BuildState.hpp"
#include "State/CompilerTools.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Environment.hpp"
#include "Terminal/Output.hpp"
#include "Terminal/Path.hpp"
#include "Utility/List.hpp"
// #include "Utility/Timer.hpp"
#include "Utility/Hash.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
IModuleStrategy::IModuleStrategy(BuildState& inState) :
	m_state(inState),
	kRootModule{ "__root_module__" }
{
}

/*****************************************************************************/
[[nodiscard]] ModuleStrategy IModuleStrategy::make(const ToolchainType inType, BuildState& inState)
{
	switch (inType)
	{
		case ToolchainType::VisualStudio:
			return std::make_unique<ModuleStrategyMSVC>(inState);
		default:
			break;
	}

	Diagnostic::errorAbort("Unimplemented ModuleStrategy requested for toolchain: ", static_cast<int>(inType));
	return nullptr;
}

/*****************************************************************************/
bool IModuleStrategy::buildProject(const SourceTarget& inProject, Unique<SourceOutputs>&& inOutputs, CompileToolchain&& inToolchain)
{
	m_sourcesChanged = false;
	m_generateDependencies = !Environment::isContinuousIntegrationServer() && !m_state.environment->isMsvc();
	m_oldStrategy = m_state.toolchain.strategy();
	m_state.toolchain.setStrategy(StrategyType::Native);
	m_rootModule.clear();
	m_previousSource.clear();

	auto cwd = String::toLowerCase(Commands::getWorkingDirectory());
	Path::sanitize(cwd);
	if (cwd.back() != '/')
		cwd += '/';

	Dictionary<ModuleLookup> modules;

	auto moduleId = getModuleId();

	auto onFailure = [this]() -> bool {
		m_state.toolchain.setStrategy(m_oldStrategy);
		m_state.cache.file().setDisallowSave(true);
		return false;
	};

	CommandPool::Settings settings;
	settings.color = Output::theme().build;
	settings.msvcCommand = m_state.environment->isMsvc();
	settings.keepGoing = m_state.info.keepGoing();
	settings.showCommands = Output::showCommands();
	settings.quiet = Output::quietNonBuild();

	// const auto pchTarget = m_state.paths.getPrecompiledHeaderTarget(inProject);

	CommandPool::Job target;
	// target.pre = getPchCommands(pchTarget);
	target.list = getModuleCommands(*inToolchain, inOutputs->groups, Dictionary<ModulePayload>{}, ModuleFileType::ModuleDependency);
	if (!target.list.empty())
	{
		// Scan sources for module dependencies

		// Output::msgScanningForModuleDependencies();
		// Output::lineBreak();

		CommandPool commandPool(m_state.info.maxJobs());
		if (!commandPool.run(target, settings))
			return onFailure();

		Output::lineBreak();
	}

	// Read dependency files for header units and determine build order
	// Build header units (build order shouldn't matter)

	// Timer timer;

	if (!readModuleDependencies(*inOutputs, modules))
		return onFailure();

	Dictionary<ModulePayload> modulePayload;
	StringList headerUnitObjects;
	SourceFileGroupList headerUnitList;

	{
		auto& sourceCache = m_state.cache.file().sources();
		const auto& objDir = m_state.paths.objDir();

		StringList addedHeaderUnits;
		for (auto& [name, module] : modules)
		{
			if (String::equals(name, kRootModule))
				m_rootModule = module.source;

			modulePayload[module.source] = ModulePayload();

			if (!addHeaderUnitsRecursively(module, module, modules, modulePayload))
				return onFailure();

			bool rebuildFromHeader = false;
			for (auto& header : module.importedHeaderUnits)
			{
				std::string file;

				auto group = std::make_unique<SourceFileGroup>();
				if (auto f = String::toLowerCase(header); String::startsWith(cwd, f))
				{
					file = header.substr(cwd.size());
					auto p = String::getPathFolder(file);
					auto dir = fmt::format("{}/{}", objDir, p);
					if (!Commands::pathExists(dir))
						Commands::makeDirectory(dir);

					header = file;

					group->sourceFile = file;
					group->dataType = SourceDataType::UserHeaderUnit;
				}
				else
				{
					file = String::getPathFilename(header);
					file = fmt::format("{}_{}", file, moduleId);

					group->sourceFile = header;
					group->dataType = SourceDataType::SystemHeaderUnit;
				}

				{
					if (m_compileCache.find(header) == m_compileCache.end())
						m_compileCache[header] = false;

					rebuildFromHeader |= sourceCache.fileChangedOrDoesNotExist(header) || m_compileCache[header];
				}

				auto ifcFile = m_state.environment->getModuleBinaryInterfaceFile(file);

				List::addIfDoesNotExist(modulePayload[module.source].headerUnitTranslations, fmt::format("{}={}", header, ifcFile));

				if (List::contains(addedHeaderUnits, header))
					continue;

				addedHeaderUnits.push_back(header);

				group->type = SourceType::CPlusPlus;
				group->objectFile = m_state.environment->getObjectFile(file);
				group->dependencyFile = m_state.environment->getModuleDirectivesDependencyFile(file);
				group->otherFile = ifcFile;

				headerUnitObjects.emplace_back(group->objectFile);
				headerUnitList.emplace_back(std::move(group));
			}

			m_compileCache[module.source] |= rebuildFromHeader;
		}
	}

	/*for (auto& [source, data] : modulePayload)
	{
		LOG(source);

		LOG("  Imported modules:");
		for (auto& item : data.moduleTranslations)
			LOG("    ", item);

		LOG("  Imported headers:");
		for (auto& item : data.headerUnitTranslations)
			LOG("    ", item);
	}*/

	target.list = getModuleCommands(*inToolchain, headerUnitList, modulePayload, ModuleFileType::HeaderUnitDependency);
	if (!target.list.empty())
	{
		// Scan sources for module dependencies

		// Output::msgBuildingRequiredHeaderUnits();
		// Output::lineBreak();

		CommandPool commandPool(m_state.info.maxJobs());
		if (!commandPool.run(target, settings))
			return onFailure();

		Output::lineBreak();
	}

	// Header units compiled first
	StringList headerUnitTranslations;

	target.list.clear();
	CommandPool::JobList buildJobs;
	{
		for (const auto& group : headerUnitList)
		{
			if (group->dataType == SourceDataType::UserHeaderUnit)
			{
				group->dependencyFile = m_state.environment->getModuleBinaryInterfaceDependencyFile(group->sourceFile);
			}
			else
			{
				auto file = String::getPathFilename(group->sourceFile);
				group->dependencyFile = m_state.environment->getModuleBinaryInterfaceDependencyFile(fmt::format("{}_{}", file, moduleId));
			}
		}

		auto job = std::make_unique<CommandPool::Job>();
		job->list = getModuleCommands(*inToolchain, headerUnitList, modulePayload, ModuleFileType::HeaderUnitObject);
		if (!job->list.empty())
		{
			buildJobs.emplace_back(std::move(job));
		}
	}
	headerUnitList.clear(); // No longer needed

	{
		auto addSourceGroup = [&](SourceFileGroup* inGroup, SourceFileGroupList& outList) {
			if (inGroup->type != SourceType::CPlusPlus)
				return;

			auto group = std::make_unique<SourceFileGroup>();
			group->type = SourceType::CPlusPlus;
			group->sourceFile = inGroup->sourceFile;
			group->objectFile = inGroup->objectFile;
			group->dependencyFile = m_state.environment->getModuleBinaryInterfaceDependencyFile(inGroup->sourceFile);
			group->otherFile = m_state.environment->getModuleBinaryInterfaceFile(inGroup->sourceFile);

			outList.emplace_back(std::move(group));
		};

		auto makeBatch = [&](const SourceFileGroupList& inList) {
			if (inList.empty())
				return;

			auto job = std::make_unique<CommandPool::Job>();
			job->list = getModuleCommands(*inToolchain, inList, modulePayload, ModuleFileType::ModuleObject);
			if (!job->list.empty())
			{
				buildJobs.emplace_back(std::move(job));
			}
		};

		SourceFileGroupList sourceCompiles;
		DependencyGraph dependencyGraph;
		{
			Dictionary<SourceFileGroup*> outGroups;
			for (auto& group : inOutputs->groups)
			{
				if (group->type != SourceType::CPlusPlus)
					continue;

				outGroups[group->sourceFile] = group.get();
			}

			for (const auto& [name, module] : modules)
			{
				if (outGroups.find(module.source) == outGroups.end())
					continue;

				dependencyGraph[outGroups.at(module.source)] = {};

				for (auto& m : module.importedModules)
				{
					if (modules.find(m) == modules.end())
						continue;

					const auto& otherModule = modules.at(m);
					dependencyGraph[outGroups.at(module.source)].push_back(outGroups.at(otherModule.source));
				}
			}
		}

		// Check for dependency changes
		{
			std::vector<SourceFileGroup*> needsRebuild;
			for (auto& [group, dependencies] : dependencyGraph)
			{
				if (m_compileCache[group->sourceFile])
					needsRebuild.push_back(group);
			}

			auto dependencyGraphCopy = dependencyGraph;
			auto itr = dependencyGraphCopy.begin();
			while (itr != dependencyGraphCopy.end())
			{
				if (!itr->second.empty())
				{
					bool erased = false;
					for (auto dep : itr->second)
					{
						if (!List::contains(needsRebuild, dep))
							continue;

						auto group = itr->first;
						m_compileCache[group->sourceFile] = true;
						needsRebuild.push_back(group);
						itr = dependencyGraphCopy.erase(itr);
						itr = dependencyGraphCopy.begin(); // We need to rescan
						erased = true;
						break;
					}
					if (!erased)
						++itr;
				}
				else
				{
					// no dependencies - we don't care about it
					itr = dependencyGraphCopy.erase(itr);
				}
			}
		}

		//

		std::vector<SourceFileGroup*> groupsAdded;

		{
			auto itr = dependencyGraph.begin();
			while (itr != dependencyGraph.end())
			{
				if (itr->second.empty())
				{
					auto group = itr->first;
					addSourceGroup(group, sourceCompiles);
					groupsAdded.push_back(group);
					itr = dependencyGraph.erase(itr);
				}
				else
					++itr;
			}
		}

		if (!sourceCompiles.empty())
		{
			/*LOG("group:");
			for (auto& group : sourceCompiles)
			{
				LOG("  ", group->sourceFile);
			}*/
			makeBatch(sourceCompiles);
			sourceCompiles.clear();
		}

		std::vector<SourceFileGroup*> addedThisLoop;
		auto itr = dependencyGraph.begin();
		while (!dependencyGraph.empty())
		{
			bool canAdd = true;
			for (auto& dep : itr->second)
			{
				canAdd &= (List::contains(groupsAdded, dep) && !List::contains(addedThisLoop, dep));
			}

			if (canAdd)
			{
				auto group = itr->first;
				addSourceGroup(group, sourceCompiles);
				groupsAdded.push_back(group);
				addedThisLoop.push_back(group);
				itr = dependencyGraph.erase(itr);
			}
			else
				++itr;

			if (itr == dependencyGraph.end())
			{
				addedThisLoop.clear();
				if (!sourceCompiles.empty())
				{
					// LOG("group:");
					for (auto& group : sourceCompiles)
					{
						// LOG("  ", group->sourceFile);
						groupsAdded.push_back(group.get());
					}

					makeBatch(sourceCompiles);
					sourceCompiles.clear();
				}
				itr = dependencyGraph.begin();
			}
		}
	}

	modules.clear(); // No longer needed

	//

	if (!buildJobs.empty())
	{
		addCompileCommands(buildJobs.back()->list, *inToolchain, inOutputs->groups);
	}
	else
	{
		auto job = std::make_unique<CommandPool::Job>();
		addCompileCommands(job->list, *inToolchain, inOutputs->groups);
		if (!job->list.empty())
		{
			buildJobs.emplace_back(std::move(job));
		}
	}

	bool targetExists = Commands::pathExists(inOutputs->target);
	if (!buildJobs.empty() || !targetExists || rebuildRequiredFromLinks(inProject))
	{
		// Scan sources for module dependencies

		// Output::msgModulesCompiling();
		// Output::lineBreak();

		settings.startIndex = 1;
		settings.total = 0;

		StringList links = List::combine(std::move(inOutputs->objectListLinker), std::move(headerUnitObjects));

		{
			auto job = std::make_unique<CommandPool::Job>();
			job->list = getLinkCommand(*inToolchain, inProject, inOutputs->target, links);
			buildJobs.emplace_back(std::move(job));
		}

		inOutputs.reset();
		inToolchain.reset();
		m_compileCache.clear();

		CommandPool commandPool(m_state.info.maxJobs());
		if (!commandPool.runAll(buildJobs, settings))
			return onFailure();

		Output::lineBreak();
	}

	// Build in groups after dependencies / order have been resolved

	m_state.toolchain.setStrategy(m_oldStrategy);

	return true;
}

/*****************************************************************************/
std::string IModuleStrategy::getBuildOutputForFile(const SourceFileGroup& inSource, const bool inIsObject)
{
	return inIsObject ? inSource.sourceFile : inSource.dependencyFile;
}

/*****************************************************************************/
CommandPool::CmdList IModuleStrategy::getModuleCommands(CompileToolchainController& inToolchain, const SourceFileGroupList& inGroups, const Dictionary<ModulePayload>& inModules, const ModuleFileType inType)
{
	auto& sourceCache = m_state.cache.file().sources();

	StringList blankList;
	CommandPool::CmdList ret;

	bool isObject = inType == ModuleFileType::ModuleObject || inType == ModuleFileType::HeaderUnitObject;

	for (auto& group : inGroups)
	{
		const auto& source = group->sourceFile;

		if (source.empty())
			continue;

		const auto& target = group->objectFile;
		const auto& dependency = group->dependencyFile;

		if (group->type != SourceType::CPlusPlus)
			continue;

		if (m_compileCache.find(source) == m_compileCache.end())
			m_compileCache[source] = false;

		bool sourceChanged = sourceCache.fileChangedOrDoesNotExist(source, isObject ? target : dependency) || m_compileCache[source];
		m_sourcesChanged |= sourceChanged;
		if (sourceChanged)
		{
			auto interfaceFile = group->otherFile;
			if (interfaceFile.empty())
			{
				interfaceFile = m_state.environment->getModuleBinaryInterfaceFile(source);
			}

			CommandPool::Cmd out;
			out.output = getBuildOutputForFile(*group, isObject);

			ModuleFileType type = inType;
			if (inType == ModuleFileType::ModuleObject && String::equals(m_rootModule, source))
				type = ModuleFileType::ModuleObjectRoot;

			if (inModules.find(source) != inModules.end())
			{
				const auto& module = inModules.at(source);

				out.command = inToolchain.compilerCxx->getModuleCommand(source, target, dependency, interfaceFile, module.moduleTranslations, module.headerUnitTranslations, type);
			}
			else
			{
				out.command = inToolchain.compilerCxx->getModuleCommand(source, target, dependency, interfaceFile, blankList, blankList, type);
			}

#if defined(CHALET_WIN32)
			if (m_state.environment->isMsvc())
				out.outputReplace = String::getPathFilename(source);
#endif

			ret.emplace_back(std::move(out));
		}

		m_compileCache[source] = sourceChanged;
	}

	return ret;
}

/*****************************************************************************/
void IModuleStrategy::addCompileCommands(CommandPool::CmdList& outList, CompileToolchainController& inToolchain, const SourceFileGroupList& inGroups)
{
	auto& sourceCache = m_state.cache.file().sources();

	for (auto& group : inGroups)
	{
		const auto& source = group->sourceFile;

		if (source.empty())
			continue;

		const auto& target = group->objectFile;
		const auto& dependency = group->dependencyFile;

		if (group->type == SourceType::WindowsResource)
		{
			if (m_compileCache.find(source) == m_compileCache.end())
				m_compileCache[source] = false;

			bool sourceChanged = sourceCache.fileChangedOrDoesNotExist(source, target) || m_compileCache[source];
			m_sourcesChanged |= sourceChanged;
			if (sourceChanged)
			{
				CommandPool::Cmd out;
				out.output = source;
				out.command = inToolchain.compilerWindowsResource->getCommand(source, target, m_generateDependencies, dependency);

#if defined(CHALET_WIN32)
				if (m_state.environment->isMsvc())
					out.outputReplace = String::getPathFilename(out.output);
#endif

				outList.emplace_back(std::move(out));
			}
			m_compileCache[source] = sourceChanged;
		}
	}
}

/*****************************************************************************/
CommandPool::CmdList IModuleStrategy::getLinkCommand(CompileToolchainController& inToolchain, const SourceTarget& inProject, const std::string& inTarget, const StringList& inLinks)
{
	const auto targetBasename = m_state.paths.getTargetBasename(inProject);

	CommandPool::CmdList ret;

	{
		CommandPool::Cmd out;
		out.command = inToolchain.getOutputTargetCommand(inTarget, inLinks, targetBasename);

		auto label = inProject.isStaticLibrary() ? "Archiving" : "Linking";
		out.output = fmt::format("{} {}", label, inTarget);

		ret.emplace_back(std::move(out));
	}

	return ret;
}

/*****************************************************************************/
bool IModuleStrategy::addHeaderUnitsRecursively(ModuleLookup& outModule, const ModuleLookup& inModule, const Dictionary<ModuleLookup>& inModules, Dictionary<ModulePayload>& outPayload)
{
	for (const auto& imported : inModule.importedModules)
	{
		if (inModules.find(imported) == inModules.end())
			continue;

		const auto& otherModule = inModules.at(imported);

		auto ifcFile = m_state.environment->getModuleBinaryInterfaceFile(otherModule.source);

		List::addIfDoesNotExist(outPayload[outModule.source].moduleTranslations, fmt::format("{}={}", imported, ifcFile));

		for (auto& header : otherModule.importedHeaderUnits)
		{
			List::addIfDoesNotExist(outModule.importedHeaderUnits, header);
		}

		if (String::equals(otherModule.source, outModule.source))
			continue;

		// LOG(m_previousSource, outModule.source, inModule.source);
		if (!m_previousSource.empty() && String::equals(otherModule.source, m_previousSource))
		{
			auto error = Output::getAnsiStyle(Output::theme().error);
			auto reset = Output::getAnsiStyle(Color::Reset);

			auto failure = fmt::format("{}FAILED: {}Cannot build the following source file due to a cyclical dependency: {} depends on {} depends on {}", error, reset, inModule.source, m_previousSource, inModule.source);
			std::cout.write(failure.data(), failure.size());
			std::cout.put(std::cout.widen('\n'));
			std::cout.flush();

			Output::lineBreak();

			return false;
		}

		m_previousSource = inModule.source;
		if (!addHeaderUnitsRecursively(outModule, otherModule, inModules, outPayload))
			return false;
	}

	m_previousSource.clear();
	return true;
}

/*****************************************************************************/
std::string IModuleStrategy::getModuleId() const
{
	std::string ret;
	const auto& hostArch = m_state.info.hostArchitectureString();
	const auto targetArch = m_state.info.targetArchitectureTriple();
	const auto envId = m_state.environment->identifier() + m_state.toolchain.version();
	const auto& buildConfig = m_state.info.buildConfiguration();

	ret = fmt::format("{}_{}_{}_{}", hostArch, targetArch, envId, buildConfig);

	return Hash::string(ret);
}

/*****************************************************************************/
bool IModuleStrategy::rebuildRequiredFromLinks(const SourceTarget& inProject) const
{
	auto& sourceCache = m_state.cache.file().sources();

	bool result = false;

	for (auto& target : m_state.targets)
	{
		if (target->isSources())
		{
			auto& project = static_cast<const SourceTarget&>(*target);

			if (String::equals(project.name(), inProject.name()))
				break;

			if (List::contains(inProject.projectStaticLinks(), project.name()))
			{
				result |= sourceCache.fileChangedOrDoesNotExist(m_state.paths.getTargetFilename(project));
			}
		}
	}
	return result;
}

}
