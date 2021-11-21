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
#include "Utility/String.hpp"
#include "Json/JsonComments.hpp"

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

	Diagnostic::errorAbort("Unimplemented ModuleStrategy requested for toolchain: ", static_cast<int>(inType));
	return nullptr;
}

/*****************************************************************************/
bool IModuleStrategy::buildProject(const SourceTarget& inProject, SourceOutputs&& inOutputs, CompileToolchain& inToolchain)
{
	m_sourcesChanged = false;
	m_generateDependencies = !Environment::isContinuousIntegrationServer() && !m_state.environment->isMsvc();
	m_oldStrategy = m_state.toolchain.strategy();
	m_state.toolchain.setStrategy(StrategyType::Native);
	m_rootModule.clear();

	if (m_msvcToolsDirectory.empty())
	{
		m_msvcToolsDirectory = Environment::getAsString("VCToolsInstallDir");
		Path::sanitize(m_msvcToolsDirectory);
	}

	const std::string kRootModule{ "__root_module__" };

	const std::string kKeyVersion{ "Version" };
	const std::string kKeyData{ "Data" };
	// const std::string kKeyIncludes{ "Includes" };
	const std::string kKeyProvidedModule{ "ProvidedModule" };
	const std::string kKeyImportedModules{ "ImportedModules" };
	const std::string kKeyImportedHeaderUnits{ "ImportedHeaderUnits" };

	// const std::string kKeyBMI{ "BMI" };

	const auto& objDir = m_state.paths.objDir();

	auto cwd = String::toLowerCase(Commands::getWorkingDirectory());
	Path::sanitize(cwd);
	if (cwd.back() != '/')
		cwd += '/';

	/*for (auto& group : inOutputs.groups)
	{
		if (group->type != SourceType::CPlusPlus)
			continue;

		auto dependencyFile = fmt::format("{}/{}.ifc.d.json", objDir, group->sourceFile);

		if (!Commands::pathExists(dependencyFile))
			continue;

		Json json;
		if (!JsonComments::parse(json, group->dependencyFile))
			continue;

		if (!json.contains(kKeyVersion) || !json.at(kKeyVersion).is_string())
			continue;

		std::string version = json.at(kKeyVersion).get<std::string>();
		if (!String::equals("1.1", version))
			continue;

		if (!json.contains(kKeyData) || !json.at(kKeyData).is_object())
			continue;

		const auto& data = json.at(kKeyData);
		if (!data.contains(kKeyIncludes) || !data.at(kKeyIncludes).is_array())
			continue;

		if (!data.contains(kKeyImportedModules) || !data.at(kKeyImportedModules).is_array())
			continue;

		if (!data.contains(kKeyImportedHeaderUnits) || !data.at(kKeyImportedHeaderUnits).is_array())
			continue;

		for (auto& moduleItr : data.at(kKeyIncludes).items())
		{
			auto& mod = moduleItr.value();
			if (!mod.is_string())
				break;
		}

		for (auto& moduleItr : data.at(kKeyImportedModules).items())
		{
			auto& mod = moduleItr.value();
			if (!mod.is_object())
				break;

			// Name / BMI
			if (mod.contains(
			auto bmi = mod.at
		}

		for (auto& fileItr : data.at(kKeyImportedHeaderUnits).items())
		{
			auto& file = fileItr.value();
			if (!file.is_object())
				break;

			// Header / BMI
		}
	}*/

	Dictionary<ModuleLookup> modules;
	Dictionary<ModulePayload> modulePayload;

	auto moduleId = m_state.getModuleId();

	auto onFailure = [this]() -> bool {
		m_state.toolchain.setStrategy(m_oldStrategy);
		return false;
	};

	CommandPool::Settings settings;
	settings.color = Output::theme().build;
	settings.msvcCommand = m_state.environment->isMsvc();
	settings.showCommands = Output::showCommands();
	settings.quiet = Output::quietNonBuild();
	settings.renameAfterCommand = false;

	// const auto pchTarget = m_state.paths.getPrecompiledHeaderTarget(inProject);

	CommandPool::Target target;
	// target.pre = getPchCommands(pchTarget);
	target.list = getModuleCommands(*inToolchain, inOutputs.groups, modulePayload, ModuleFileType::ModuleDependency);

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

	// Version 1.1

	for (auto& group : inOutputs.groups)
	{
		if (group->type != SourceType::CPlusPlus)
			continue;

		if (!Commands::pathExists(group->dependencyFile))
			continue;

		Json json;
		if (!JsonComments::parse(json, group->dependencyFile))
		{
			Diagnostic::error("Failed to parse: {}", group->dependencyFile);
			return onFailure();
		}

		if (!json.contains(kKeyVersion) || !json.at(kKeyVersion).is_string())
		{
			Diagnostic::error("{}: Missing expected key '{}'", group->dependencyFile, kKeyVersion);
			return onFailure();
		}

		std::string version = json.at(kKeyVersion).get<std::string>();
		if (!String::equals("1.1", version))
		{
			Diagnostic::error("{}: Found version '{}', but only '1.1' is supported", group->dependencyFile, version);
			return onFailure();
		}

		if (!json.contains(kKeyData) || !json.at(kKeyData).is_object())
		{
			Diagnostic::error("{}: Missing expected key '{}'", group->dependencyFile, kKeyData);
			return onFailure();
		}

		const auto& data = json.at(kKeyData);
		if (!data.contains(kKeyProvidedModule) || !data.at(kKeyProvidedModule).is_string())
		{
			Diagnostic::error("{}: Missing expected key '{}'", group->dependencyFile, kKeyProvidedModule);
			return onFailure();
		}

		if (!data.contains(kKeyImportedModules) || !data.at(kKeyImportedModules).is_array())
		{
			Diagnostic::error("{}: Missing expected key '{}'", group->dependencyFile, kKeyImportedModules);
			return onFailure();
		}

		if (!data.contains(kKeyImportedHeaderUnits) || !data.at(kKeyImportedHeaderUnits).is_array())
		{
			Diagnostic::error("{}: Missing expected key '{}'", group->dependencyFile, kKeyImportedHeaderUnits);
			return onFailure();
		}

		auto name = data.at(kKeyProvidedModule).get<std::string>();
		if (name.empty())
			name = kRootModule;

		modules[name].source = group->sourceFile;

		for (auto& moduleItr : data.at(kKeyImportedModules).items())
		{
			auto& mod = moduleItr.value();
			if (!mod.is_string())
			{
				Diagnostic::error("{}: Unexpected structure for '{}'", group->dependencyFile, kKeyImportedModules);
				return onFailure();
			}

			List::addIfDoesNotExist(modules[name].importedModules, mod.get<std::string>());
		}

		for (auto& fileItr : data.at(kKeyImportedHeaderUnits).items())
		{
			auto& file = fileItr.value();
			if (!file.is_string())
			{
				Diagnostic::error("{}: Unexpected structure for '{}'", group->dependencyFile, kKeyImportedHeaderUnits);
				return onFailure();
			}

			auto outHeader = file.get<std::string>();
			Path::sanitize(outHeader);

			List::addIfDoesNotExist(modules[name].importedHeaderUnits, std::move(outHeader));
		}
	}

	StringList headerUnitObjects;
	SourceFileGroupList headerUnitList;

	{
		StringList addedHeaderUnits;
		for (auto& [name, module] : modules)
		{
			if (String::equals(name, kRootModule))
				m_rootModule = module.source;

			modulePayload[module.source] = ModulePayload();

			addHeaderUnitsRecursively(module, module, modules, modulePayload);

			for (const auto& header : module.importedHeaderUnits)
			{
				auto file = String::getPathFilename(header);
				auto ifcFile = fmt::format("{}/{}_{}.ifc", objDir, file, moduleId);

				List::addIfDoesNotExist(modulePayload[module.source].headerUnitTranslations, fmt::format("{}={}", header, ifcFile));

				if (List::contains(addedHeaderUnits, header))
					continue;

				addedHeaderUnits.push_back(header);

				auto group = std::make_unique<SourceFileGroup>();
				group->type = SourceType::CPlusPlus;
				group->sourceFile = header;
				group->objectFile = fmt::format("{}/{}_{}.obj", objDir, file, moduleId);
				group->dependencyFile = fmt::format("{}/{}_{}.module.json", objDir, file, moduleId);
				group->otherFile = ifcFile;

				headerUnitObjects.emplace_back(group->objectFile);
				headerUnitList.emplace_back(std::move(group));
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
	std::vector<Unique<CommandPool::Target>> buildBatches;
	{
		for (const auto& group : headerUnitList)
		{
			auto file = String::getPathFilename(group->sourceFile);
			group->dependencyFile = fmt::format("{}/{}_{}.ifc.d.json", objDir, file, moduleId);
		}

		auto batch = std::make_unique<CommandPool::Target>();
		batch->list = getModuleCommands(*inToolchain, headerUnitList, modulePayload, ModuleFileType::HeaderUnitObject);
		if (!batch->list.empty())
		{
			buildBatches.emplace_back(std::move(batch));
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
			group->dependencyFile = fmt::format("{}/{}.ifc.d.json", objDir, inGroup->sourceFile);
			group->otherFile = fmt::format("{}/{}.ifc", objDir, inGroup->sourceFile);

			outList.emplace_back(std::move(group));
		};

		auto makeBatch = [&](const SourceFileGroupList& inList, const uint threads) {
			if (inList.empty())
				return;

			auto batch = std::make_unique<CommandPool::Target>();
			batch->threads = threads;
			batch->list = getModuleCommands(*inToolchain, inList, modulePayload, ModuleFileType::ModuleObject);
			if (!batch->list.empty())
			{
				buildBatches.emplace_back(std::move(batch));
			}
		};

		SourceFileGroupList sourceCompiles;
		DependencyGraph dependencyGraph;
		{
			Dictionary<SourceFileGroup*> outGroups;
			for (auto& group : inOutputs.groups)
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
			makeBatch(sourceCompiles, 0); // multi-threaded - because these don't have any dependencies
			sourceCompiles.clear();
		}

		auto itr = dependencyGraph.begin();
		while (!dependencyGraph.empty())
		{
			bool canAdd = true;
			for (auto& dep : itr->second)
			{
				canAdd &= List::contains(groupsAdded, dep);
			}

			if (canAdd)
			{
				auto group = itr->first;
				addSourceGroup(group, sourceCompiles);
				groupsAdded.push_back(group);
				itr = dependencyGraph.erase(itr);
			}
			else
				++itr;

			if (itr == dependencyGraph.end())
			{
				if (!sourceCompiles.empty())
				{
					// LOG("group:");
					for (auto& group : sourceCompiles)
					{
						// LOG("  ", group->sourceFile);
						groupsAdded.push_back(group.get());
					}

					// TODO: These batches are single-threaded for now, because there can still be dependency collisions between files
					//   aka one command could be writing files while another tries to read them
					//
					makeBatch(sourceCompiles, 1); // single threaded
					sourceCompiles.clear();
				}
				itr = dependencyGraph.begin();
			}
		}
	}

	modules.clear(); // No longer needed

	//

	if (!buildBatches.empty())
	{
		addCompileCommands(buildBatches.back()->list, *inToolchain, inOutputs.groups);
	}
	else
	{
		auto batch = std::make_unique<CommandPool::Target>();
		addCompileCommands(batch->list, *inToolchain, inOutputs.groups);
		if (!batch->list.empty())
		{
			buildBatches.emplace_back(std::move(batch));
		}
	}

	bool targetExists = Commands::pathExists(inOutputs.target);
	if (!buildBatches.empty() || !targetExists)
	{
		// Scan sources for module dependencies

		// Output::msgModulesCompiling();
		// Output::lineBreak();

		settings.startIndex = 1;
		settings.total = 0;

		StringList links = List::combine(std::move(inOutputs.objectListLinker), std::move(headerUnitObjects));

		if (!buildBatches.empty())
		{
			buildBatches.back()->post = getLinkCommand(*inToolchain, inProject, inOutputs.target, links);

			for (auto& batch : buildBatches)
			{
				settings.total += static_cast<uint>(batch->list.size());
				if (!batch->post.command.empty())
					settings.total += 1;
			}
		}
		else
		{
			auto batch = std::make_unique<CommandPool::Target>();
			batch->post = getLinkCommand(*inToolchain, inProject, inOutputs.target, links);
			buildBatches.emplace_back(std::move(batch));

			settings.total = 1;
		}

		for (auto& batch : buildBatches)
		{
			CommandPool commandPool(batch->threads == 0 ? m_state.info.maxJobs() : batch->threads);
			if (!commandPool.run(*batch, settings))
				return onFailure();

			settings.startIndex += static_cast<uint>(batch->list.size());
		}

		// Output::lineBreak();
	}

	// Build in groups after dependencies / order have been resolved

	m_state.toolchain.setStrategy(m_oldStrategy);

	return true;
}

/*****************************************************************************/
void IModuleStrategy::addHeaderUnitsRecursively(ModuleLookup& outModule, const ModuleLookup& inModule, const Dictionary<ModuleLookup>& inModules, Dictionary<ModulePayload>& outPayload)
{
	const auto& objDir = m_state.paths.objDir();

	for (const auto& imported : inModule.importedModules)
	{
		if (inModules.find(imported) == inModules.end())
			continue;

		const auto& otherModule = inModules.at(imported);

		auto ifcFile = fmt::format("{}/{}.ifc", objDir, otherModule.source);

		List::addIfDoesNotExist(outPayload[outModule.source].moduleTranslations, fmt::format("{}={}", imported, ifcFile));

		for (auto& header : otherModule.importedHeaderUnits)
		{
			List::addIfDoesNotExist(outModule.importedHeaderUnits, header);
		}

		addHeaderUnitsRecursively(outModule, otherModule, inModules, outPayload);
	}
}

/*****************************************************************************/
CommandPool::CmdList IModuleStrategy::getModuleCommands(CompileToolchainController& inToolchain, const SourceFileGroupList& inGroups, const Dictionary<ModulePayload>& inModules, const ModuleFileType inType)
{
	auto& sourceCache = m_state.cache.file().sources();

	CommandPool::CmdList ret;
	const auto& objDir = m_state.paths.objDir();

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
				interfaceFile = fmt::format("{}/{}.ifc", objDir, source);
			}

			CommandPool::Cmd out;
			out.output = isObject ? source : dependency;
			if (String::startsWith(m_msvcToolsDirectory, out.output))
			{
				out.output = String::getPathFilename(out.output);
			}

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
				StringList blank;
				out.command = inToolchain.compilerCxx->getModuleCommand(source, target, dependency, interfaceFile, blank, blank, type);
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
CommandPool::Cmd IModuleStrategy::getLinkCommand(CompileToolchainController& inToolchain, const SourceTarget& inProject, const std::string& inTarget, const StringList& inLinks)
{
	const auto targetBasename = m_state.paths.getTargetBasename(inProject);

	CommandPool::Cmd ret;
	ret.command = inToolchain.getOutputTargetCommand(inTarget, inLinks, targetBasename);
	ret.label = inProject.isStaticLibrary() ? "Archiving" : "Linking";
	ret.output = inTarget;

	return ret;
}

}
