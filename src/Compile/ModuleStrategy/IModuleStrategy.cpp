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
#include "Utility/Hash.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"
// #include "Utility/Timer.hpp"

namespace chalet
{
/*****************************************************************************/
IModuleStrategy::IModuleStrategy(BuildState& inState) :
	m_state(inState)
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

	Diagnostic::errorAbort("Unimplemented ModuleStrategy requested: {}", static_cast<int>(inType));
	return nullptr;
}

/*****************************************************************************/
bool IModuleStrategy::buildProject(const SourceTarget& inProject, Unique<SourceOutputs>&& inOutputs, CompileToolchain&& inToolchain)
{
	m_sourcesChanged = false;
	m_generateDependencies = !Environment::isContinuousIntegrationServer() && !m_state.environment->isMsvc();
	m_oldStrategy = m_state.toolchain.strategy();
	m_state.toolchain.setStrategy(StrategyType::Native);
	m_implementationUnits.clear();
	m_previousSource.clear();

	auto cwd = String::toLowerCase(Commands::getWorkingDirectory());
	Path::sanitize(cwd);
	if (cwd.back() != '/')
		cwd += '/';

	Dictionary<ModuleLookup> modules;

	m_moduleId = getModuleId();

	auto onFailure = [this]() -> bool {
		Output::lineBreak();

		m_state.toolchain.setStrategy(m_oldStrategy);
		// m_state.cache.file().setDisallowSave(true);
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
		std::vector<ModuleLookup*> systemModules;
		for (auto& [name, module] : modules)
		{
			if (module.systemModule)
				systemModules.emplace_back(&module);
		}

		if (!systemModules.empty())
		{
			auto& cppStandard = inProject.cppStandard();
			if (String::equals("c++20", cppStandard))
			{
				StringList sysModList;
				for (auto& module : systemModules)
				{

					sysModList.emplace_back(String::getPathBaseName(module->source));
				}

				Diagnostic::error("This project requires cppStandard=c++23 at a minimum because it imports the standard library module(s): {}", String::join(sysModList, ','));
				Diagnostic::printErrors();

				m_state.toolchain.setStrategy(m_oldStrategy);
				return false;
			}
			// Requires C++23

			for (auto& module : systemModules)
			{
				modulePayload[module->source] = ModulePayload();

				auto baseName = String::getPathBaseName(module->source);
				baseName = fmt::format("{}_{}", baseName, m_moduleId);
				m_systemModules[module->source] = baseName;

				auto group = std::make_unique<SourceFileGroup>();
				group->type = SourceType::CPlusPlus;
				group->sourceFile = module->source;
				group->objectFile = m_state.environment->getObjectFile(baseName);
				group->dependencyFile = m_state.environment->getModuleBinaryInterfaceDependencyFile(baseName);
				group->otherFile = m_state.environment->getModuleBinaryInterfaceFile(baseName);

				inOutputs->groups.emplace_back(std::move(group));
			}
		}
	}

	// Do this line break after the std module check
	if (!target.list.empty())
		Output::lineBreak();

	{
		auto& sourceCache = m_state.cache.file().sources();
		const auto& objDir = m_state.paths.objDir();

		StringList addedHeaderUnits;
		for (auto& [name, module] : modules)
		{
			if (String::startsWith('@', name))
				m_implementationUnits.push_back(module.source);

			modulePayload[module.source] = ModulePayload();

			if (module.systemModule)
				continue;

			if (!addModuleRecursively(module, module, modules, modulePayload))
				return onFailure();

			bool rebuildFromHeader = false;
			for (auto& header : module.importedHeaderUnits)
			{
				std::string file;

				auto group = std::make_unique<SourceFileGroup>();
				if (isSystemModuleFile(header))
				{
					file = String::getPathFilename(header);
					file = fmt::format("{}_{}", file, m_moduleId);

					group->sourceFile = header;
					group->dataType = SourceDataType::SystemHeaderUnit;
				}
				else
				{
					auto lowerHeader = String::toLowerCase(header);
					if (String::startsWith(cwd, lowerHeader))
						file = header.substr(cwd.size());
					else
						file = header;

					auto p = String::getPathFolder(file);
					auto dir = fmt::format("{}/{}", objDir, p);
					if (!Commands::pathExists(dir))
						Commands::makeDirectory(dir);

					header = file;

					group->sourceFile = file;
					group->dataType = SourceDataType::UserHeaderUnit;
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

	// Scan Includes deduced from the .d.json files
	{
		bool rebuildFromIncludes = false;
		auto& sourceCache = m_state.cache.file().sources();
		for (auto& group : inOutputs->groups)
		{
			if (group->type != SourceType::CPlusPlus)
				continue;

			const auto& sourceFile = group->sourceFile;
			rebuildFromIncludes |= sourceCache.fileChangedOrDoesNotExist(sourceFile) || m_compileCache[sourceFile];
			if (!rebuildFromIncludes)
			{
				std::string dependencyFile;
				if (isSystemModuleFile(sourceFile))
					dependencyFile = group->dependencyFile;
				else
					dependencyFile = m_state.environment->getModuleBinaryInterfaceDependencyFile(sourceFile);

				if (!dependencyFile.empty() && Commands::pathExists(dependencyFile))
				{
					StringList includes;
					if (!readIncludesFromDependencyFile(dependencyFile, includes))
						continue;

					for (auto& include : includes)
					{
						rebuildFromIncludes |= sourceCache.fileChangedOrDoesNotExist(include) || m_compileCache[sourceFile];
					}
					m_compileCache[sourceFile] |= rebuildFromIncludes;
				}
			}
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
				group->dependencyFile = m_state.environment->getModuleBinaryInterfaceDependencyFile(fmt::format("{}_{}", file, m_moduleId));
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
		auto addSourceGroup = [this](SourceFileGroup* inGroup, SourceFileGroupList& outList) {
			if (inGroup->type != SourceType::CPlusPlus)
				return;

			auto group = std::make_unique<SourceFileGroup>();
			group->type = SourceType::CPlusPlus;
			group->sourceFile = inGroup->sourceFile;
			if (isSystemModuleFile(group->sourceFile))
			{
				auto& file = m_systemModules.at(inGroup->sourceFile);
				group->objectFile = m_state.environment->getObjectFile(file);
				group->dependencyFile = m_state.environment->getModuleBinaryInterfaceDependencyFile(file);
				group->otherFile = m_state.environment->getModuleBinaryInterfaceFile(file);
			}
			else
			{
				group->objectFile = inGroup->objectFile;
				group->dependencyFile = m_state.environment->getModuleBinaryInterfaceDependencyFile(group->sourceFile);
				group->otherFile = m_state.environment->getModuleBinaryInterfaceFile(group->sourceFile);
			}

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
					if (outGroups.find(otherModule.source) == outGroups.end())
						continue;

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

	// No longer needed
	modules.clear();
	m_systemModules.clear();

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
	bool requiredFromLinks = rebuildRequiredFromLinks(inProject);
	// LOG("modules can build:", !buildJobs.empty(), !targetExists, requiredFromLinks);
	if (!buildJobs.empty() || !targetExists || requiredFromLinks)
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
		{
			auto& sourceCache = m_state.cache.file().sources();
			for (auto& failure : commandPool.failures())
			{
				auto objectFile = m_state.environment->getObjectFile(failure);

				if (Commands::pathExists(objectFile))
					Commands::remove(objectFile);

				sourceCache.markForLater(failure);
			}
			return onFailure();
		}

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
			if (inType == ModuleFileType::ModuleObject && List::contains(m_implementationUnits, source))
				type = ModuleFileType::ModuleImplementationUnit;

			if (inModules.find(source) != inModules.end())
			{
				const auto& module = inModules.at(source);

				out.command = inToolchain.compilerCxx->getModuleCommand(source, target, dependency, interfaceFile, module.moduleTranslations, module.headerUnitTranslations, type);
			}
			else
			{
				out.command = inToolchain.compilerCxx->getModuleCommand(source, target, dependency, interfaceFile, blankList, blankList, type);
			}

			out.reference = source;

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

				out.reference = String::getPathFilename(out.output);

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
bool IModuleStrategy::addModuleRecursively(ModuleLookup& outModule, const ModuleLookup& inModule, const Dictionary<ModuleLookup>& inModules, Dictionary<ModulePayload>& outPayload)
{
	for (const auto& imported : inModule.importedModules)
	{
		if (inModules.find(imported) == inModules.end())
			continue;

		const auto& otherModule = inModules.at(imported);

		std::string ifcFile;
		if (m_systemModules.find(otherModule.source) != m_systemModules.end())
		{
			ifcFile = m_state.environment->getModuleBinaryInterfaceFile(m_systemModules.at(otherModule.source));
		}
		else
		{
			ifcFile = m_state.environment->getModuleBinaryInterfaceFile(otherModule.source);
		}

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
			auto reset = Output::getAnsiStyle(Output::theme().reset);

			auto failure = fmt::format("{}FAILED: {}Cannot build the following source file due to a cyclical dependency: {} depends on {} depends on {}", error, reset, inModule.source, m_previousSource, inModule.source);
			std::cout.write(failure.data(), failure.size());
			std::cout.put('\n');
			std::cout.flush();

			Output::lineBreak();

			return false;
		}

		m_previousSource = inModule.source;
		if (!addModuleRecursively(outModule, otherModule, inModules, outPayload))
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
