/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/ModuleStrategy/IModuleStrategy.hpp"

#include "BuildEnvironment/IBuildEnvironment.hpp"
#include "Cache/SourceCache.hpp"
#include "Cache/WorkspaceCache.hpp"
#include "Compile/CompileCommandsGenerator.hpp"
#include "Compile/ModuleStrategy/ModuleStrategyMSVC.hpp"
#include "Core/CommandLineInputs.hpp"
#include "State/BuildInfo.hpp"
#include "State/BuildPaths.hpp"
#include "State/BuildState.hpp"
#include "State/CompilerTools.hpp"
#include "System/Files.hpp"
#include "Terminal/Output.hpp"
#include "Terminal/Shell.hpp"
#include "Utility/Hash.hpp"
#include "Utility/List.hpp"
#include "Utility/Path.hpp"
#include "Utility/String.hpp"
// #include "Utility/Timer.hpp"

namespace chalet
{
/*****************************************************************************/
IModuleStrategy::IModuleStrategy(BuildState& inState, CompileCommandsGenerator& inCompileCommandsGenerator) :
	m_state(inState),
	m_compileCommandsGenerator(inCompileCommandsGenerator)
{
}

/*****************************************************************************/
[[nodiscard]] ModuleStrategy IModuleStrategy::make(const ToolchainType inType, BuildState& inState, CompileCommandsGenerator& inCompileCommandsGenerator)
{
	switch (inType)
	{
		case ToolchainType::VisualStudio:
			return std::make_unique<ModuleStrategyMSVC>(inState, inCompileCommandsGenerator);
		default:
			break;
	}

	Diagnostic::errorAbort("Unimplemented ModuleStrategy requested: {}", static_cast<i32>(inType));
	return nullptr;
}

/*****************************************************************************/
bool IModuleStrategy::buildProject(const SourceTarget& inProject, Unique<SourceOutputs>&& inOutputs, CompileToolchain&& inToolchain)
{
	m_sourcesChanged = false;
	m_oldStrategy = m_state.toolchain.strategy();
	m_state.toolchain.setStrategy(StrategyType::Native);
	m_implementationUnits.clear();
	m_previousSource.clear();
	m_moduleId = getModuleId();

	checkCommandsForChanges(inProject, *inToolchain);

	Dictionary<ModuleLookup> modules;

	CommandPool::Job target;

	// 1. Generate module source file dependency files to determine build order later
	//
	if (!scanSourcesForModuleDependencies(target, *inToolchain, inOutputs->groups))
		return onFailure();

	// 2. Read the module dependency files that were generated
	//
	if (!readModuleDependencies(*inOutputs, modules))
		return onFailure();

	// Build header units (build order shouldn't matter)
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
				group->dataType = SourceDataType::SystemModule;
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
		auto cwd = m_state.inputs.workingDirectory() + '/';

		auto& sourceCache = m_state.cache.file().sources();
		const auto& objDir = m_state.paths.objDir();

		StringList addedHeaderUnits;
		SourceFileGroupList userHeaderUnits;
		for (auto& [name, module] : modules)
		{
			if (String::startsWith('@', name))
				m_implementationUnits.push_back(module.source);

			modulePayload[module.source] = ModulePayload();

			if (!addModuleRecursively(module, module, modules, modulePayload))
				return onFailure();

			if (module.systemModule)
				continue;

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
					if (String::startsWith(cwd, header))
						file = header.substr(cwd.size());
					else
						file = header;

					auto p = String::getPathFolder(file);
					auto dir = fmt::format("{}/{}", objDir, p);
					if (!Files::pathExists(dir))
						Files::makeDirectory(dir);

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

				if (group->dataType == SourceDataType::UserHeaderUnit)
				{
					userHeaderUnits.emplace_back(std::move(group));
				}
				else
				{
					headerUnitObjects.emplace_back(group->objectFile);
					headerUnitList.emplace_back(std::move(group));
				}
			}

			m_compileCache[module.source] |= rebuildFromHeader;
		}

		// Sort user header units at the end of headerUnitList
		//
		auto itr = userHeaderUnits.begin();
		while (itr != userHeaderUnits.end())
		{
			auto&& group = *itr;
			headerUnitObjects.emplace_back(group->objectFile);
			headerUnitList.emplace_back(std::move(group));
			itr = userHeaderUnits.erase(itr);
		}
	}

	// Scan Includes deduced from the dependency files
	checkIncludedHeaderFilesForChanges(inOutputs->groups);

	// Log the current payload
	// logPayload(modulePayload);

	// Generate module header unit dependency files to determine their build order
	//
	if (!scanHeaderUnitsForModuleDependencies(target, *inToolchain, modulePayload, headerUnitList))
		return onFailure();

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

		checkForDependencyChanges(dependencyGraph);
		addModuleBuildJobs(*inToolchain, modulePayload, sourceCompiles, dependencyGraph, buildJobs);
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

	bool targetExists = Files::pathExists(inOutputs->target);
	bool requiredFromLinks = rebuildRequiredFromLinks(inProject);
	// LOG("modules can build:", !buildJobs.empty(), !targetExists, requiredFromLinks);
	if (m_targetCommandChanged || !buildJobs.empty() || !targetExists || requiredFromLinks)
	{
		// Scan sources for module dependencies

		// Output::msgModulesCompiling();
		// Output::lineBreak();

		auto settings = getCommandPoolSettings();
		settings.startIndex = 1;
		settings.total = 0;

		StringList links = List::combineRemoveDuplicates(std::move(inOutputs->objectListLinker), std::move(headerUnitObjects));

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
			for (auto& failure : commandPool.failures())
			{
				auto objectFile = m_state.environment->getObjectFile(failure);
				Files::removeIfExists(objectFile);
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
std::string IModuleStrategy::getBuildOutputForFile(const SourceFileGroup& inSource, const bool inIsObject) const
{
	if (!inIsObject)
		return inSource.dependencyFile;

	if (inSource.dataType == SourceDataType::SystemHeaderUnit || inSource.dataType == SourceDataType::SystemModule)
		return String::getPathFilename(inSource.sourceFile);

	return inSource.sourceFile;
}

/*****************************************************************************/
CommandPool::CmdList IModuleStrategy::getModuleCommands(CompileToolchainController& inToolchain, const SourceFileGroupList& inGroups, const Dictionary<ModulePayload>& inModules, const ModuleFileType inType) const
{
	auto& sourceCache = m_state.cache.file().sources();

	StringList blankList;
	CommandPool::CmdList ret;

	bool isObject = inType == ModuleFileType::ModuleObject || inType == ModuleFileType::HeaderUnitObject;

	for (auto& group : inGroups)
	{
		if (group->type != SourceType::CPlusPlus)
			continue;

		const auto& source = group->sourceFile;
		if (source.empty())
			continue;

		const auto& target = group->objectFile;
		const auto& dependency = group->dependencyFile;

		if (m_compileCache.find(source) == m_compileCache.end())
			m_compileCache[source] = false;

		bool sourceChanged = m_moduleCommandsChanged || sourceCache.fileChangedOrDoesNotExist(source, isObject ? target : dependency) || m_compileCache[source];
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
				out.reference = source;
				addToCompileCommandsJson(out);
			}
			else
			{
				out.command = inToolchain.compilerCxx->getModuleCommand(source, target, dependency, interfaceFile, blankList, blankList, type);
				out.reference = source;
				if (type == ModuleFileType::HeaderUnitObject)
				{
					addToCompileCommandsJson(out);
				}
			}

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

			bool sourceChanged = m_winResourceCommandsChanged || sourceCache.fileChangedOrDoesNotExist(source, target) || m_compileCache[source];
			m_sourcesChanged |= sourceChanged;
			if (sourceChanged)
			{
				CommandPool::Cmd out;
				out.output = source;
				out.command = inToolchain.compilerWindowsResource->getCommand(source, target, dependency);

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
	CommandPool::CmdList ret;

	{
		CommandPool::Cmd out;
		out.command = inToolchain.getOutputTargetCommand(inTarget, inLinks);

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
				result |= m_moduleCommandsChanged || sourceCache.fileChangedOrDoesNotExist(m_state.paths.getTargetFilename(project));
			}
		}
	}
	return result;
}

/*****************************************************************************/
bool IModuleStrategy::onFailure()
{
	Output::lineBreak();

	m_state.toolchain.setStrategy(m_oldStrategy);
	return false;
}

/*****************************************************************************/
CommandPool::Settings IModuleStrategy::getCommandPoolSettings() const
{
	CommandPool::Settings ret;
	ret.color = Output::theme().build;
	ret.msvcCommand = m_state.environment->isMsvc();
	ret.keepGoing = m_state.info.keepGoing();
	ret.showCommands = Output::showCommands();
	ret.quiet = Output::quietNonBuild();

	return ret;
}

/*****************************************************************************/
bool IModuleStrategy::scanSourcesForModuleDependencies(CommandPool::Job& outJob, CompileToolchainController& inToolchain, const SourceFileGroupList& inGroups) const
{
	// Scan sources for module dependencies

	outJob.list = getModuleCommands(inToolchain, inGroups, Dictionary<ModulePayload>{}, ModuleFileType::ModuleDependency);
	if (!outJob.list.empty())
	{
		// Output::msgScanningForModuleDependencies();
		// Output::lineBreak();

		auto settings = getCommandPoolSettings();
		CommandPool commandPool(m_state.info.maxJobs());
		if (!commandPool.run(outJob, settings))
		{
			auto& failures = commandPool.failures();
			for (auto& failure : failures)
			{
				auto dependency = m_state.environment->getModuleDirectivesDependencyFile(failure);
				Files::removeIfExists(dependency);
			}
			return false;
		}
	}

	return true;
}

/*****************************************************************************/
void IModuleStrategy::checkIncludedHeaderFilesForChanges(const SourceFileGroupList& inGroups)
{
	bool rebuildFromIncludes = false;
	auto& sourceCache = m_state.cache.file().sources();
	for (auto& group : inGroups)
	{
		if (group->type != SourceType::CPlusPlus)
			continue;

		const auto& sourceFile = group->sourceFile;
		rebuildFromIncludes |= m_moduleCommandsChanged || sourceCache.fileChangedOrDoesNotExist(sourceFile) || m_compileCache[sourceFile];
		if (!rebuildFromIncludes)
		{
			std::string dependencyFile;
			if (isSystemModuleFile(sourceFile))
				dependencyFile = group->dependencyFile;
			else
				dependencyFile = m_state.environment->getModuleBinaryInterfaceDependencyFile(sourceFile);

			if (!dependencyFile.empty() && Files::pathExists(dependencyFile))
			{
				StringList includes;
				if (!readIncludesFromDependencyFile(dependencyFile, includes))
					continue;

				for (auto& include : includes)
				{
					rebuildFromIncludes |= m_moduleCommandsChanged || sourceCache.fileChangedOrDoesNotExist(include) || m_compileCache[sourceFile];
				}
				m_compileCache[sourceFile] |= rebuildFromIncludes;
			}
		}
	}
}

/*****************************************************************************/
bool IModuleStrategy::scanHeaderUnitsForModuleDependencies(CommandPool::Job& outJob, CompileToolchainController& inToolchain, Dictionary<ModulePayload>& outPayload, const SourceFileGroupList& inGroups) const
{
	outJob.list = getModuleCommands(inToolchain, inGroups, outPayload, ModuleFileType::HeaderUnitDependency);
	if (!outJob.list.empty())
	{
		// Scan sources for module dependencies

		// Output::msgBuildingRequiredHeaderUnits();
		// Output::lineBreak();

		CommandPool commandPool(m_state.info.maxJobs());
		if (!commandPool.run(outJob, getCommandPoolSettings()))
		{
			auto& failures = commandPool.failures();
			for (auto& failure : failures)
			{
				auto dependency = m_state.environment->getModuleDirectivesDependencyFile(failure);
				Files::removeIfExists(dependency);
			}

			return false;
		}

		Output::lineBreak();
	}

	return true;
}

/*****************************************************************************/
void IModuleStrategy::checkForDependencyChanges(DependencyGraph& inDependencyGraph) const
{
	std::vector<SourceFileGroup*> needsRebuild;
	for (auto& [group, dependencies] : inDependencyGraph)
	{
		if (m_compileCache[group->sourceFile])
			needsRebuild.push_back(group);
	}

	auto dependencyGraphCopy = inDependencyGraph;
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

/*****************************************************************************/
bool IModuleStrategy::addSourceGroup(SourceFileGroup* inGroup, SourceFileGroupList& outList) const
{
	if (inGroup->type != SourceType::CPlusPlus)
		return false;

	auto group = std::make_unique<SourceFileGroup>();
	group->type = SourceType::CPlusPlus;
	group->dataType = inGroup->dataType;
	group->sourceFile = inGroup->sourceFile;

	if (group->dataType == SourceDataType::SystemModule)
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
	return true;
}

/*****************************************************************************/
bool IModuleStrategy::makeModuleBatch(CompileToolchainController& inToolchain, const Dictionary<ModulePayload>& inModules, const SourceFileGroupList& inList, CommandPool::JobList& outJobList) const
{
	if (inList.empty())
		return false;

	auto job = std::make_unique<CommandPool::Job>();
	job->list = getModuleCommands(inToolchain, inList, inModules, ModuleFileType::ModuleObject);
	if (!job->list.empty())
	{
		outJobList.emplace_back(std::move(job));
	}

	return true;
}

/*****************************************************************************/
std::vector<SourceFileGroup*> IModuleStrategy::getSourceFileGroupsForBuild(DependencyGraph& outDependencyGraph, SourceFileGroupList& outList) const
{
	std::vector<SourceFileGroup*> ret;

	auto itr = outDependencyGraph.begin();
	while (itr != outDependencyGraph.end())
	{
		if (itr->second.empty())
		{
			auto group = itr->first;
			addSourceGroup(group, outList);
			ret.push_back(group);
			itr = outDependencyGraph.erase(itr);
		}
		else
			++itr;
	}

	return ret;
}

/*****************************************************************************/
void IModuleStrategy::addModuleBuildJobs(CompileToolchainController& inToolchain, const Dictionary<ModulePayload>& inModules, SourceFileGroupList& sourceCompiles, DependencyGraph& outDependencyGraph, CommandPool::JobList& outJobList) const
{
	auto groupsAdded = getSourceFileGroupsForBuild(outDependencyGraph, sourceCompiles);
	if (!sourceCompiles.empty())
	{
		/*LOG("group:");
		for (auto& group : sourceCompiles)
		{
			LOG("  ", group->sourceFile);
		}*/
		makeModuleBatch(inToolchain, inModules, sourceCompiles, outJobList);
		sourceCompiles.clear();
	}

	std::vector<SourceFileGroup*> addedThisLoop;
	auto itr = outDependencyGraph.begin();
	while (!outDependencyGraph.empty())
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
			itr = outDependencyGraph.erase(itr);
		}
		else
			++itr;

		if (itr == outDependencyGraph.end())
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

				makeModuleBatch(inToolchain, inModules, sourceCompiles, outJobList);
				sourceCompiles.clear();
			}
			itr = outDependencyGraph.begin();
		}
	}
}

/*****************************************************************************/
void IModuleStrategy::logPayload(const Dictionary<ModulePayload>& inPayload) const
{
	for (auto& [source, data] : inPayload)
	{
		LOG(source);

		LOG("  Imported modules:");
		for (auto& item : data.moduleTranslations)
			LOG("    ", item);

		LOG("  Imported headers:");
		for (auto& item : data.headerUnitTranslations)
			LOG("    ", item);
	}
}

/*****************************************************************************/
void IModuleStrategy::checkCommandsForChanges(const SourceTarget& inProject, CompileToolchainController& inToolchain)
{
	m_moduleCommandsChanged = false;
	m_winResourceCommandsChanged = false;
	m_targetCommandChanged = false;

	auto& name = inProject.name();
	auto& sourceCache = m_state.cache.file().sources();

	{
		StringList moduleTranslations{};
		StringList headerUnitTranslations{};
		std::string sourceFile{ "cmd.cppm" };
		auto objectFile = String::getPathFilename(m_state.environment->getObjectFile(sourceFile));
		auto dependencyFile = String::getPathFilename(m_state.environment->getDependencyFile(sourceFile));
		auto interfaceFile = String::getPathFilename(m_state.environment->getModuleBinaryInterfaceFile(sourceFile));

		{
			auto type = ModuleFileType::ModuleObject;
			auto cxxHashKey = Hash::string(fmt::format("{}_cxx_module_{}", name, static_cast<int>(type)));
			StringList options = inToolchain.compilerCxx->getModuleCommand(sourceFile, objectFile, dependencyFile, interfaceFile, moduleTranslations, headerUnitTranslations, type);

			auto hash = Hash::string(String::join(options));
			m_moduleCommandsChanged = sourceCache.dataCacheValueChanged(cxxHashKey, hash);
		}

		{
			auto cxxHashKey = Hash::string(fmt::format("{}_source_{}", name, static_cast<int>(SourceType::WindowsResource)));
			StringList options = inToolchain.compilerWindowsResource->getCommand("cmd.rc", "cmd.res", "cmd.rc.d");

			auto hash = Hash::string(String::join(options));
			m_winResourceCommandsChanged = sourceCache.dataCacheValueChanged(cxxHashKey, hash);
		}

		auto targetHashKey = Hash::string(fmt::format("{}_target", name));
		auto targetOptions = inToolchain.getOutputTargetCommand(inProject.outputFile(), StringList{});
		auto targetHash = Hash::string(String::join(targetOptions));
		m_targetCommandChanged = sourceCache.dataCacheValueChanged(targetHashKey, targetHash);
	}
}

/*****************************************************************************/
void IModuleStrategy::addToCompileCommandsJson(const CommandPool::Cmd& inCmd) const
{
	if (m_state.info.generateCompileCommands())
	{
		m_compileCommandsGenerator.addCompileCommand(inCmd.reference, StringList(inCmd.command));
	}
}

}
