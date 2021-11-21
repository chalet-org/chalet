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

	const auto& objDir = m_state.paths.objDir();
	const auto& modulesDir = m_state.paths.modulesDir();

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

	auto cwd = String::toLowerCase(Commands::getWorkingDirectory());
	Path::sanitize(cwd);
	if (cwd.back() != '/')
		cwd += '/';

	// const auto pchTarget = m_state.paths.getPrecompiledHeaderTarget(inProject);

	CommandPool::Target target;
	// target.pre = getPchCommands(pchTarget);
	target.list = getModuleCommands(*inToolchain, inOutputs.groups, modulePayload, ModuleFileType::ModuleDependency);

	if (!target.list.empty())
	{
		// Scan sources for module dependencies

		Output::msgScanningForModuleDependencies();
		Output::lineBreak();

		CommandPool commandPool(m_state.info.maxJobs());
		if (!commandPool.run(target, settings))
			return onFailure();

		Output::lineBreak();
	}

	// Read dependency files for header units and determine build order
	// Build header units (build order shouldn't matter)

	const std::string kRootModule{ "__root_module__" };

	// Timer timer;

	// Version 1.1

	const std::string kKeyVersion{ "Version" };
	const std::string kKeyData{ "Data" };
	const std::string kKeySource{ "Source" };
	const std::string kKeyProvidedModule{ "ProvidedModule" };
	const std::string kKeyImportedModules{ "ImportedModules" };
	const std::string kKeyImportedHeaderUnits{ "ImportedHeaderUnits" };

	for (auto& group : inOutputs.groups)
	{
		if (group->type == SourceType::CPlusPlus)
		{
			Json json;
			if (!Commands::pathExists(group->dependencyFile))
				continue;

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
			if (!data.contains(kKeySource) || !data.at(kKeySource).is_string())
			{
				Diagnostic::error("{}: Missing expected key '{}'", group->dependencyFile, kKeySource);
				return onFailure();
			}

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

			modules[name].source = data.at(kKeySource).get<std::string>();
			Path::sanitize(modules[name].source);
			String::replaceAll(modules[name].source, cwd, "");

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
	}

	StringList addedHeaderUnits;
	StringList headerUnitObjects;
	SourceFileGroupList headerUnitList;

	for (auto& [name, module] : modules)
	{
		if (String::equals(name, kRootModule))
			m_rootModule = module.source;

		modulePayload[module.source] = ModulePayload();

		for (const auto& imported : module.importedModules)
		{
			if (modules.find(imported) == modules.end())
				continue;

			const auto& otherModule = modules.at(imported);

			auto filename = String::getPathFilename(otherModule.source);
			auto ifcFile = fmt::format("{}/{}.ifc", modulesDir, filename);

			List::addIfDoesNotExist(modulePayload[module.source].moduleTranslations, fmt::format("{}={}", imported, ifcFile));

			for (auto& header : otherModule.importedHeaderUnits)
			{
				List::addIfDoesNotExist(module.importedHeaderUnits, header);
			}
		}

		for (const auto& header : module.importedHeaderUnits)
		{
			auto file = String::getPathFilename(header);
			auto ifcFile = fmt::format("{}/{}_{}.ifc", modulesDir, file, moduleId);

			List::addIfDoesNotExist(modulePayload[module.source].headerUnitTranslations, fmt::format("{}={}", header, ifcFile));

			if (List::contains(addedHeaderUnits, header))
				continue;

			addedHeaderUnits.push_back(header);

			auto group = std::make_unique<chalet::SourceFileGroup>();
			group->type = SourceType::CPlusPlus;
			group->sourceFile = header;
			group->objectFile = fmt::format("{}/{}_{}.obj", objDir, file, moduleId);
			group->dependencyFile = fmt::format("{}/{}_{}.module.json", modulesDir, file, moduleId);
			group->otherFile = ifcFile;

			headerUnitObjects.emplace_back(group->objectFile);
			headerUnitList.emplace_back(std::move(group));
		}
	}

	modules.clear(); // No longer needed

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

		Output::msgBuildingRequiredHeaderUnits();
		Output::lineBreak();

		CommandPool commandPool(m_state.info.maxJobs());
		if (!commandPool.run(target, settings))
			return onFailure();

		Output::lineBreak();
	}

	// Header units compiled first
	StringList headerUnitTranslations;

	target.list.clear();
	std::vector<Unique<CommandPool::Target>> buildPortions;
	{
		for (const auto& group : headerUnitList)
		{
			auto file = String::getPathFilename(group->sourceFile);
			group->dependencyFile = fmt::format("{}/{}_{}.ifc.d.json", modulesDir, file, moduleId);
		}

		auto portion = std::make_unique<CommandPool::Target>();
		portion->list = getModuleCommands(*inToolchain, headerUnitList, modulePayload, ModuleFileType::HeaderUnitObject);
		if (!portion->list.empty())
		{
			buildPortions.emplace_back(std::move(portion));
		}
	}
	headerUnitList.clear(); // No longer needed

	{
		std::unique_ptr<SourceFileGroup> rootModule;
		SourceFileGroupList sourceCompiles;
		for (auto& inGroup : inOutputs.groups)
		{
			if (inGroup->type != SourceType::CPlusPlus)
				continue;

			auto file = String::getPathFilename(inGroup->sourceFile);

			auto group = std::make_unique<chalet::SourceFileGroup>();
			group->type = SourceType::CPlusPlus;
			group->sourceFile = inGroup->sourceFile;
			group->objectFile = inGroup->objectFile;
			group->dependencyFile = fmt::format("{}/{}.ifc.d.json", modulesDir, file);
			group->otherFile = fmt::format("{}/{}.ifc", modulesDir, file);

			if (String::equals(inGroup->sourceFile, m_rootModule))
			{
				rootModule = std::move(group);
			}
			else
			{
				sourceCompiles.emplace_back(std::move(group));
			}
		}

		auto portion = std::make_unique<CommandPool::Target>();
		portion->list = getModuleCommands(*inToolchain, sourceCompiles, modulePayload, ModuleFileType::ModuleObject);
		if (!portion->list.empty())
		{
			buildPortions.emplace_back(std::move(portion));
		}

		if (rootModule != nullptr)
		{
			sourceCompiles.clear();
			sourceCompiles.emplace_back(std::move(rootModule));

			portion = std::make_unique<CommandPool::Target>();
			portion->list = getModuleCommands(*inToolchain, sourceCompiles, modulePayload, ModuleFileType::ModuleObject);
			if (!portion->list.empty())
			{
				buildPortions.emplace_back(std::move(portion));
			}
		}
	}

	//

	if (!buildPortions.empty())
	{
		addCompileCommands(buildPortions.back()->list, *inToolchain, inOutputs.groups);
	}
	else
	{
		auto portion = std::make_unique<CommandPool::Target>();
		addCompileCommands(portion->list, *inToolchain, inOutputs.groups);
		if (!portion->list.empty())
		{
			buildPortions.emplace_back(std::move(portion));
		}
	}

	bool targetExists = Commands::pathExists(inOutputs.target);
	if (!buildPortions.empty() || !targetExists)
	{
		// Scan sources for module dependencies

		Output::msgModulesCompiling();
		Output::lineBreak();

		settings.startIndex = 1;
		settings.total = 0;

		StringList links = List::combine(std::move(inOutputs.objectListLinker), std::move(headerUnitObjects));

		if (!buildPortions.empty())
		{
			buildPortions.back()->post = getLinkCommand(*inToolchain, inProject, inOutputs.target, links);

			for (auto& portion : buildPortions)
			{
				settings.total += static_cast<uint>(portion->list.size());
				if (!portion->post.command.empty())
					settings.total += 1;
			}
		}
		else
		{
			auto portion = std::make_unique<CommandPool::Target>();
			portion->post = getLinkCommand(*inToolchain, inProject, inOutputs.target, links);
			buildPortions.emplace_back(std::move(portion));

			settings.total = 1;
		}

		for (auto& portion : buildPortions)
		{
			CommandPool commandPool(m_state.info.maxJobs());
			if (!commandPool.run(*portion, settings))
				return onFailure();

			settings.startIndex += static_cast<uint>(portion->list.size());
		}

		Output::lineBreak();
	}

	// Build in groups after dependencies / order have been resolved

	m_state.toolchain.setStrategy(m_oldStrategy);
	UNUSED(inProject, inToolchain);
	return true;
}

/*****************************************************************************/
CommandPool::CmdList IModuleStrategy::getModuleCommands(CompileToolchainController& inToolchain, const SourceFileGroupList& inGroups, const Dictionary<ModulePayload>& inModules, const ModuleFileType inType)
{
	auto& sourceCache = m_state.cache.file().sources();

	CommandPool::CmdList ret;
	const auto& modulesDir = m_state.paths.modulesDir();

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

		bool sourceChanged = m_compileCache[source] || sourceCache.fileChangedOrDoesNotExist(source, isObject ? target : dependency);
		m_sourcesChanged |= sourceChanged;
		if (sourceChanged)
		{
			auto interfaceFile = group->otherFile;
			if (interfaceFile.empty())
			{
				interfaceFile = fmt::format("{}/{}.ifc", modulesDir, source);
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

		m_compileCache[source] = m_sourcesChanged;
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

			bool sourceChanged = m_compileCache[source] || sourceCache.fileChangedOrDoesNotExist(source, target);
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
			m_compileCache[source] = m_sourcesChanged;
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
