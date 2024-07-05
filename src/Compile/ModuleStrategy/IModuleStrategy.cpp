/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/ModuleStrategy/IModuleStrategy.hpp"

#include "BuildEnvironment/IBuildEnvironment.hpp"
#include "Cache/SourceCache.hpp"
#include "Cache/WorkspaceCache.hpp"
#include "Compile/CompileCommandsGenerator.hpp"
#include "Compile/ModuleStrategy/ModuleStrategyClang.hpp"
#include "Compile/ModuleStrategy/ModuleStrategyGCC.hpp"
#include "Compile/ModuleStrategy/ModuleStrategyMSVC.hpp"
#include "Core/CommandLineInputs.hpp"
#include "State/BuildInfo.hpp"
#include "State/BuildPaths.hpp"
#include "State/BuildState.hpp"
#include "State/CompilerTools.hpp"
#include "State/Target/CMakeTarget.hpp"
#include "State/Target/SubChaletTarget.hpp"
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
	m_compileCommandsGenerator(inCompileCommandsGenerator),
	m_compileAdapter(inState)
{
}

/*****************************************************************************/
[[nodiscard]] ModuleStrategy IModuleStrategy::make(const ToolchainType inType, BuildState& inState, CompileCommandsGenerator& inCompileCommandsGenerator)
{
	switch (inType)
	{
		case ToolchainType::VisualStudio:
			return std::make_unique<ModuleStrategyMSVC>(inState, inCompileCommandsGenerator);
		case ToolchainType::GNU:
		case ToolchainType::MingwGNU:
			return std::make_unique<ModuleStrategyGCC>(inState, inCompileCommandsGenerator);
		case ToolchainType::LLVM:
		case ToolchainType::AppleLLVM:
		case ToolchainType::IntelLLVM:
		case ToolchainType::MingwLLVM:
		case ToolchainType::VisualStudioLLVM:
		case ToolchainType::Emscripten:
			return std::make_unique<ModuleStrategyClang>(inState, inCompileCommandsGenerator);
		default:
			break;
	}

	Diagnostic::error("Unimplemented ModuleStrategy requested: {}", static_cast<i32>(inType));
	return nullptr;
}

/*****************************************************************************/
bool IModuleStrategy::initialize()
{
	const auto& compiler = m_state.toolchain.compilerCpp().path;
	m_systemHeaderDirectories = m_state.environment->getSystemIncludeDirectories(compiler);

	return true;
}

/*****************************************************************************/
bool IModuleStrategy::buildProject(const SourceTarget& inProject)
{
	if (!initialize())
		return false;

	m_sourcesChanged = false;
	m_oldStrategy = m_state.toolchain.strategy();
	m_state.toolchain.setStrategy(StrategyType::Native);

	m_implementationUnits.clear();
	m_previousSource.clear();
	m_moduleId = getModuleId();

	m_project = &inProject;

	checkCommandsForChanges();

	bool otherTargetsChanged = m_compileAdapter.anyCmakeOrSubChaletTargetsChanged();

	CommandPool::Job target;

	// 1. Generate module source file dependency files to determine build order later
	//
	if (!scanSourcesForModuleDependencies(target))
		return onFailure();

	// 2. Read the module dependency files that were generated
	//
	if (!readModuleDependencies())
		return onFailure();

	// Build header units (build order shouldn't matter)

	if (!addSystemModules())
		return onFailure();

	// Do this line break after the std module check
	if (!target.list.empty())
		Output::lineBreak();

	if (!addAllHeaderUnits())
		return onFailure();

	// Scan Includes deduced from the dependency files
	checkIncludedHeaderFilesForChanges();

	// Log the current payload
	// logPayload(m_modulePayload);

	// Generate module header unit dependency files to determine their build order
	//
	if (!scanHeaderUnitsForModuleDependencies(target))
		return onFailure();

	target.list.clear();

	CommandPool::JobList buildJobs;
	addHeaderUnitsBuildJob(buildJobs);

	m_headerUnitList.clear(); // No longer needed

	buildDependencyGraphAndAddModulesBuildJobs(buildJobs);

	// No longer needed
	m_modules.clear();
	m_systemModules.clear();

	//

	addOtherBuildJobsToLastJob(buildJobs);

	bool targetExists = Files::pathExists(outputs->target);
	bool requiredFromLinks = m_moduleCommandsChanged || m_compileAdapter.rebuildRequiredFromLinks(*m_project);
	// LOG("modules can build:", !buildJobs.empty(), !targetExists, requiredFromLinks);
	bool dependentChanged = targetExists && m_compileAdapter.checkDependentTargets(*m_project);
	bool linkTarget = m_targetCommandChanged || !buildJobs.empty() || requiredFromLinks || dependentChanged || otherTargetsChanged || !targetExists;
	if (linkTarget)
	{
		// Scan sources for module dependencies

		// Output::msgModulesCompiling();
		// Output::lineBreak();

		auto settings = m_compileAdapter.getCommandPoolSettings();
		settings.startIndex = 1;
		settings.total = 0;

		addHeaderUnitsToTargetLinks();

		{
			auto job = std::make_unique<CommandPool::Job>();
			job->list = m_compileAdapter.getLinkCommand(*m_project, *toolchain, *outputs);
			if (!job->list.empty())
				buildJobs.emplace_back(std::move(job));
		}

		// clear up memory
		outputs.reset();
		toolchain.reset();
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
	m_project = nullptr;

	return true;
}

/*****************************************************************************/
bool IModuleStrategy::addSystemModules()
{
	std::vector<ModuleLookup*> systemModules;
	for (auto& [name, module] : m_modules)
	{
		if (module.systemModule)
			systemModules.emplace_back(&module);
	}

	if (!systemModules.empty())
	{
		auto& cppStandard = m_project->cppStandard();
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
			m_modulePayload[module->source] = ModulePayload();

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

			outputs->groups.emplace_back(std::move(group));
		}
	}

	return true;
}

/*****************************************************************************/
bool IModuleStrategy::addAllHeaderUnits()
{
	auto cwd = m_state.inputs.workingDirectory() + '/';
	auto& includeDirs = m_project->includeDirs();

	auto& sourceCache = m_state.cache.file().sources();
	const auto& objDir = m_state.paths.objDir();

	StringList addedHeaderUnits;
	SourceFileGroupList userHeaderUnits;
	for (auto& [name, module] : m_modules)
	{
		if (module.implementationUnit)
			m_implementationUnits.push_back(module.source);

		m_modulePayload[module.source] = ModulePayload();

		if (!addModuleRecursively(module, module))
			return false;

		if (module.systemModule)
			continue;

		bool rebuildFromHeader = false;
		for (auto& header : module.importedHeaderUnits)
		{
			std::string file;
			std::string headerUnitName;

			auto group = std::make_unique<SourceFileGroup>();
			if (isSystemHeaderFileOrModuleFile(header))
			{
				if (m_state.environment->isClang())
					headerUnitName = String::getPathFilename(header);
				else
					headerUnitName = header;

				file = String::getPathFilename(header);
				file = fmt::format("{}_{}", file, m_moduleId);

				group->sourceFile = header;
				group->dataType = SourceDataType::SystemHeaderUnit;
			}
			else
			{
				if (!Files::pathExists(header))
				{
					for (auto& dir : includeDirs)
					{
						auto resolved = fmt::format("{}/{}", dir, header);
						if (Files::pathExists(resolved))
						{
							header = resolved;
							break;
						}
					}
				}

				if (String::startsWith(cwd, header))
					file = header.substr(cwd.size());
				else
					file = header;

				auto p = String::getPathFolder(file);
				auto dir = fmt::format("{}/{}", objDir, p);
				if (!Files::pathExists(dir))
					Files::makeDirectory(dir);

				header = file;
				headerUnitName = header;

				group->sourceFile = file;
				group->dataType = SourceDataType::UserHeaderUnit;
			}

			{
				if (m_compileCache.find(header) == m_compileCache.end())
					m_compileCache[header] = false;

				rebuildFromHeader |= sourceCache.fileChangedOrDoesNotExist(header) || m_compileCache[header];
			}

			auto ifcFile = m_state.environment->getModuleBinaryInterfaceFile(file);

			List::addIfDoesNotExist(m_modulePayload[module.source].headerUnitTranslations, fmt::format("{}={}", headerUnitName, ifcFile));

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
				m_headerUnitObjects.emplace_back(group->objectFile);
				m_headerUnitList.emplace_back(std::move(group));
			}
		}

		m_compileCache[module.source] |= rebuildFromHeader;
	}

	sortHeaderUnits(userHeaderUnits);

	return true;
}

/*****************************************************************************/
void IModuleStrategy::sortHeaderUnits(SourceFileGroupList& userHeaderUnits)
{
	// Sort user header units at the end of m_headerUnitList
	//
	auto itr = userHeaderUnits.begin();
	while (itr != userHeaderUnits.end())
	{
		auto&& group = *itr;
		m_headerUnitObjects.emplace_back(group->objectFile);
		m_headerUnitList.emplace_back(std::move(group));
		itr = userHeaderUnits.erase(itr);
	}
}

/*****************************************************************************/
void IModuleStrategy::addHeaderUnitsToTargetLinks()
{
	if (m_state.environment->isMsvc())
	{
		outputs->objectListLinker = List::combineRemoveDuplicates(std::move(outputs->objectListLinker), std::move(m_headerUnitObjects));
	}
}

/*****************************************************************************/
void IModuleStrategy::addHeaderUnitsBuildJob(CommandPool::JobList& jobs)
{
	for (const auto& group : m_headerUnitList)
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
	job->list = getModuleCommands(m_headerUnitList, m_modulePayload, ModuleFileType::HeaderUnitObject);
	if (!job->list.empty())
		jobs.emplace_back(std::move(job));
}

/*****************************************************************************/
void IModuleStrategy::buildDependencyGraphAndAddModulesBuildJobs(CommandPool::JobList& jobs)
{
	SourceFileGroupList sourceCompiles;
	DependencyGraph dependencyGraph;
	{
		Dictionary<SourceFileGroup*> outGroups;
		for (auto& group : outputs->groups)
		{
			if (group->type != SourceType::CPlusPlus)
				continue;

			outGroups[group->sourceFile] = group.get();
		}

		for (const auto& [name, module] : m_modules)
		{
			if (outGroups.find(module.source) == outGroups.end())
				continue;

			dependencyGraph[outGroups.at(module.source)] = {};

			for (auto& m : module.importedModules)
			{
				if (m_modules.find(m) == m_modules.end())
					continue;

				const auto& otherModule = m_modules.at(m);
				if (outGroups.find(otherModule.source) == outGroups.end())
					continue;

				dependencyGraph[outGroups.at(module.source)].push_back(outGroups.at(otherModule.source));
			}
		}
	}

	checkForDependencyChanges(dependencyGraph);
	addModulesBuildJobs(jobs, sourceCompiles, dependencyGraph);
}

/*****************************************************************************/
void IModuleStrategy::addOtherBuildJobsToLastJob(CommandPool::JobList& jobs)
{
	if (!jobs.empty())
	{
		auto& job = jobs.back();
		addOtherBuildCommands(job->list);
		if (job->list.empty())
			jobs.pop_back();
	}
	else
	{
		auto job = std::make_unique<CommandPool::Job>();
		addOtherBuildCommands(job->list);
		if (!job->list.empty())
			jobs.emplace_back(std::move(job));
	}
}

/*****************************************************************************/
ModuleFileType IModuleStrategy::getFileType(const Unique<SourceFileGroup>& group, const ModuleFileType inBaseType) const
{
	if (inBaseType == ModuleFileType::HeaderUnitDependency)
		return inBaseType;

	if (inBaseType == ModuleFileType::ModuleObject && List::contains(m_implementationUnits, group->sourceFile))
		return ModuleFileType::ModuleImplementationUnit;

	bool systemHeaderUnit = group->dataType == SourceDataType::SystemHeaderUnit;
	bool userHeaderUnit = group->dataType == SourceDataType::UserHeaderUnit;

	if (systemHeaderUnit)
		return ModuleFileType::SystemHeaderUnitObject;
	else if (userHeaderUnit)
		return ModuleFileType::HeaderUnitObject;

	return inBaseType;
}

/*****************************************************************************/
std::string IModuleStrategy::getBmiFile(const Unique<SourceFileGroup>& group)
{
	if (group->otherFile.empty())
	{
		return m_state.environment->getModuleBinaryInterfaceFile(group->sourceFile);
	}

	return group->otherFile;
}

/*****************************************************************************/
CommandPool::CmdList IModuleStrategy::getModuleCommands(const SourceFileGroupList& inGroups, const Dictionary<ModulePayload>& inPayload, const ModuleFileType inType)
{
	auto& sourceCache = m_state.cache.file().sources();

	StringList blankList;
	CommandPool::CmdList ret;

	bool isObject = inType == ModuleFileType::ModuleObject || inType == ModuleFileType::HeaderUnitObject;
	bool isHeaderUnitDependency = inType == ModuleFileType::HeaderUnitDependency;
	bool isGcc = m_state.environment->isGcc();
	bool isClang = m_state.environment->isClang();

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
			setCompilerCache(source, false);

		auto type = getFileType(group, inType);

		bool systemHeaderUnit = group->dataType == SourceDataType::SystemHeaderUnit;
		bool userHeaderUnit = group->dataType == SourceDataType::UserHeaderUnit;
		bool isHeaderUnit = systemHeaderUnit || userHeaderUnit;

		auto bmiFile = getBmiFile(group);

		// Note: don't make objectDependent a reference - breaks in MSVC
		const std::string* objectDependent = &target;
		if ((isGcc || isClang) && (isHeaderUnit || isHeaderUnitDependency))
			objectDependent = &bmiFile;
		else if (isObject)
			objectDependent = &dependency;

		bool sourceChanged = m_moduleCommandsChanged || sourceCache.fileChangedOrDoesNotExist(source, *objectDependent) || cachedValue(source);
		m_sourcesChanged |= sourceChanged;
		if (sourceChanged)
		{
			CommandPool::Cmd out;
			out.output = getBuildOutputForFile(*group, isObject);

			std::string inputFile;
			if (systemHeaderUnit && isGcc)
				inputFile = String::getPathFilename(source);
			else
				inputFile = source;

			if (inPayload.find(source) != inPayload.end())
			{
				const auto& module = inPayload.at(source);

				out.command = toolchain->compilerCxx->getModuleCommand(inputFile, target, dependency, bmiFile, module.moduleTranslations, module.headerUnitTranslations, type);
			}
			else
			{
				out.command = toolchain->compilerCxx->getModuleCommand(inputFile, target, dependency, bmiFile, blankList, blankList, type);
			}
			out.reference = source;

			if (out.command.empty())
				continue;

			ret.emplace_back(std::move(out));
		}

		setCompilerCache(source, sourceChanged);
	}

	if (m_sourcesChanged)
	{
		m_compileAdapter.addChangedTarget(*m_project);
	}

	// Generate compile commands
	if (m_state.info.generateCompileCommands())
	{
		for (auto& group : inGroups)
		{
			if (group->type != SourceType::CPlusPlus)
				continue;

			const auto& source = group->sourceFile;
			if (source.empty())
				continue;

			const auto& target = group->objectFile;
			const auto& dependency = group->dependencyFile;

			auto type = getFileType(group, inType);

			auto bmiFile = getBmiFile(group);

			if (inPayload.find(source) != inPayload.end())
			{
				const auto& module = inPayload.at(source);

				addToCompileCommandsJson(source, toolchain->compilerCxx->getModuleCommand(source, target, dependency, bmiFile, module.moduleTranslations, module.headerUnitTranslations, type));
			}
			else if (type == ModuleFileType::HeaderUnitObject)
			{
				addToCompileCommandsJson(source, toolchain->compilerCxx->getModuleCommand(source, target, dependency, bmiFile, blankList, blankList, type));
			}
		}
	}

	return ret;
}

/*****************************************************************************/
void IModuleStrategy::addOtherBuildCommands(CommandPool::CmdList& outList)
{
	auto& sourceCache = m_state.cache.file().sources();

	for (auto& group : outputs->groups)
	{
		const auto& source = group->sourceFile;

		if (source.empty())
			continue;

		const auto& target = group->objectFile;
		const auto& dependency = group->dependencyFile;

		if (group->type == SourceType::WindowsResource)
		{
			if (m_compileCache.find(source) == m_compileCache.end())
				setCompilerCache(source, false);

			bool sourceChanged = m_winResourceCommandsChanged || sourceCache.fileChangedOrDoesNotExist(source, target) || cachedValue(source);
			m_sourcesChanged |= sourceChanged;
			if (sourceChanged)
			{
				CommandPool::Cmd out;
				out.output = source;
				out.command = toolchain->compilerWindowsResource->getCommand(source, target, dependency);

				out.reference = String::getPathFilename(out.output);

				outList.emplace_back(std::move(out));
			}
			setCompilerCache(source, sourceChanged);
		}
	}
}

/*****************************************************************************/
bool IModuleStrategy::addModuleRecursively(ModuleLookup& outModule, const ModuleLookup& inModule)
{
	for (const auto& imported : inModule.importedModules)
	{
		if (m_modules.find(imported) == m_modules.end())
			continue;

		const auto& otherModule = m_modules.at(imported);

		std::string ifcFile;
		if (m_systemModules.find(otherModule.source) != m_systemModules.end())
		{
			ifcFile = m_state.environment->getModuleBinaryInterfaceFile(m_systemModules.at(otherModule.source));
		}
		else
		{
			ifcFile = m_state.environment->getModuleBinaryInterfaceFile(otherModule.source);
		}

		List::addIfDoesNotExist(m_modulePayload[outModule.source].moduleTranslations, fmt::format("{}={}", imported, ifcFile));

		for (auto& header : otherModule.importedHeaderUnits)
		{
			List::addIfDoesNotExist(outModule.importedHeaderUnits, header);
		}

		if (String::equals(otherModule.source, outModule.source))
			continue;

		// LOG(m_previousSource, outModule.source, inModule.source);
		if (!m_previousSource.empty() && String::equals(otherModule.source, m_previousSource))
		{
			const auto& error = Output::getAnsiStyle(Output::theme().error);
			const auto& reset = Output::getAnsiStyle(Output::theme().reset);

			auto failure = fmt::format("{}FAILED: {}Cannot build the following source file due to a cyclical dependency: {} depends on {} depends on {}", error, reset, inModule.source, m_previousSource, inModule.source);
			std::cout.write(failure.data(), failure.size());
			std::cout.put('\n');
			std::cout.flush();

			Output::lineBreak();

			return false;
		}

		m_previousSource = inModule.source;
		if (!addModuleRecursively(outModule, otherModule))
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
bool IModuleStrategy::isSystemHeaderFileOrModuleFile(const std::string& inFile) const
{
	if (m_systemHeaderDirectories.empty())
		return false;

	for (auto& systemDir : m_systemHeaderDirectories)
	{
		if (String::startsWith(systemDir, inFile))
			return true;
	}

	return false;
}

/*****************************************************************************/
std::string IModuleStrategy::getBuildOutputForFile(const SourceFileGroup& inFile, const bool inIsObject) const
{
	std::string ret = inIsObject ? inFile.sourceFile : inFile.dependencyFile;
	for (auto& systemDir : m_systemHeaderDirectories)
	{
		if (String::startsWith(systemDir, ret))
		{
			ret = String::getPathFilename(ret);
			break;
		}
	}

	return ret;
}

/*****************************************************************************/
bool IModuleStrategy::cachedValue(const std::string& inSource) const
{
	return m_compileCache[inSource];
}

/*****************************************************************************/
void IModuleStrategy::setCompilerCache(const std::string& inSource, const bool inValue) const
{
	m_compileCache[inSource] = inValue;
}

/*****************************************************************************/
bool IModuleStrategy::onFailure()
{
	Output::lineBreak();

	m_state.toolchain.setStrategy(m_oldStrategy);
	return false;
}

/*****************************************************************************/
void IModuleStrategy::checkIncludedHeaderFilesForChanges()
{
	bool rebuildFromIncludes = false;
	auto& sourceCache = m_state.cache.file().sources();
	for (auto& group : outputs->groups)
	{
		if (group->type != SourceType::CPlusPlus)
			continue;

		const auto& sourceFile = group->sourceFile;
		bool sourceNeedsUpdate = cachedValue(sourceFile);
		rebuildFromIncludes |= m_moduleCommandsChanged || sourceCache.fileChangedOrDoesNotExist(sourceFile);
		if (sourceNeedsUpdate || !rebuildFromIncludes)
		{
			std::string dependencyFile;
			if (isSystemHeaderFileOrModuleFile(sourceFile))
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
					rebuildFromIncludes |= sourceCache.fileChangedOrDoesNotExist(include);
				}
				setCompilerCache(sourceFile, sourceNeedsUpdate || rebuildFromIncludes);
			}
		}
	}
}

/*****************************************************************************/
void IModuleStrategy::checkForDependencyChanges(DependencyGraph& inDependencyGraph) const
{
	std::vector<SourceFileGroup*> needsRebuild;
	for (auto& [group, dependencies] : inDependencyGraph)
	{
		if (cachedValue(group->sourceFile))
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
				setCompilerCache(group->sourceFile, true);
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
bool IModuleStrategy::makeModuleBatch(CommandPool::JobList& jobs, const SourceFileGroupList& inList)
{
	if (inList.empty())
		return false;

	auto job = std::make_unique<CommandPool::Job>();
	job->list = getModuleCommands(inList, m_modulePayload, ModuleFileType::ModuleObject);
	if (!job->list.empty())
		jobs.emplace_back(std::move(job));

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
void IModuleStrategy::addModulesBuildJobs(CommandPool::JobList& jobs, SourceFileGroupList& sourceCompiles, DependencyGraph& outDependencyGraph)
{
	auto groupsAdded = getSourceFileGroupsForBuild(outDependencyGraph, sourceCompiles);
	if (!sourceCompiles.empty())
	{
		/*LOG("group:");
		for (auto& group : sourceCompiles)
		{
			LOG("  ", group->sourceFile);
		}*/
		makeModuleBatch(jobs, sourceCompiles);
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

				makeModuleBatch(jobs, sourceCompiles);
				sourceCompiles.clear();
			}
			itr = outDependencyGraph.begin();
		}
	}
}

/*****************************************************************************/
void IModuleStrategy::logPayload() const
{
	for (auto& [source, data] : m_modulePayload)
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
void IModuleStrategy::checkCommandsForChanges()
{
	m_moduleCommandsChanged = false;
	m_winResourceCommandsChanged = false;
	m_targetCommandChanged = false;

	auto& name = m_project->name();
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
			StringList options = toolchain->compilerCxx->getModuleCommand(sourceFile, objectFile, dependencyFile, interfaceFile, moduleTranslations, headerUnitTranslations, type);

			auto hash = Hash::string(String::join(options));
			m_moduleCommandsChanged = sourceCache.dataCacheValueChanged(cxxHashKey, hash);
		}

		if (toolchain->compilerWindowsResource)
		{
			auto cxxHashKey = Hash::string(fmt::format("{}_source_{}", name, static_cast<int>(SourceType::WindowsResource)));
			StringList options = toolchain->compilerWindowsResource->getCommand("cmd.rc", "cmd.res", "cmd.rc.d");

			auto hash = Hash::string(String::join(options));
			m_winResourceCommandsChanged = sourceCache.dataCacheValueChanged(cxxHashKey, hash);
		}

		auto targetHashKey = Hash::string(fmt::format("{}_target", name));
		auto targetOptions = toolchain->getOutputTargetCommand(m_project->outputFile(), m_project->files());
		auto targetHash = Hash::string(String::join(targetOptions));
		m_targetCommandChanged = sourceCache.dataCacheValueChanged(targetHashKey, targetHash);

		if (m_targetCommandChanged)
		{
			m_compileAdapter.addChangedTarget(*m_project);
		}
	}
}

/*****************************************************************************/
void IModuleStrategy::addToCompileCommandsJson(const std::string& inReference, StringList&& inCmd) const
{
	if (m_state.info.generateCompileCommands())
	{
		m_compileCommandsGenerator.addCompileCommand(inReference, std::move(inCmd));
	}
}
}
