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
bool IModuleStrategy::buildProject(const SourceTarget& inProject, const SourceOutputs& inOutputs, CompileToolchain& inToolchain)
{
	m_sourcesChanged = false;
	m_generateDependencies = !Environment::isContinuousIntegrationServer() && !m_state.environment->isMsvc();
	m_oldStrategy = m_state.toolchain.strategy();
	m_state.toolchain.setStrategy(StrategyType::Native);

	if (m_msvcToolsDirectory.empty())
	{
		m_msvcToolsDirectory = Environment::getAsString("VCToolsInstallDir");
		Path::sanitize(m_msvcToolsDirectory);
	}

	const auto& objDir = m_state.paths.objDir();
	const auto& modulesDir = m_state.paths.modulesDir();

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
	target.list = getModuleCommands(*inToolchain, inOutputs.groups, ModuleFileType::ModuleDependency);

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

	{
		// Timer timer;
		StringList headerUnits;

		// Version 1.1

		const std::string kKeyVersion{ "Version" };
		const std::string kKeyData{ "Data" };
		const std::string kKeySource{ "Source" };
		const std::string kKeyProvidedModule{ "ProvidedModule" };
		// const std::string kKeyImportedModules{ "ImportedModules" };
		const std::string kKeyImportedHeaderUnits{ "ImportedHeaderUnits" };

		std::string rootFile;

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

				std::string source = data.at(kKeySource).get<std::string>();
				std::string providedModule = data.at(kKeyProvidedModule).get<std::string>();
				if (providedModule.empty())
				{
					if (rootFile.empty())
					{
						rootFile = std::move(source);
						Path::sanitize(rootFile);
					}
					else
					{
						Diagnostic::error("{}: Found with empty '{}', but root was already found in another file.", source, kKeyProvidedModule);
						return onFailure();
					}
				}

				if (!data.contains(kKeyImportedHeaderUnits) || !data.at(kKeyImportedHeaderUnits).is_array())
				{
					Diagnostic::error("{}: Missing expected key '{}'", group->dependencyFile, kKeyImportedHeaderUnits);
					return onFailure();
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
					headerUnits.emplace_back(std::move(outHeader));
				}
			}
		}

		// LOG(fmt::format("Dependencies read in: {}", timer.asString()));

		{
			SourceFileGroupList headerUnitList;

			for (const auto& header : headerUnits)
			{
				auto file = String::getPathFilename(header);

				auto group = std::make_unique<chalet::SourceFileGroup>();
				group->type = SourceType::CPlusPlus;
				group->sourceFile = header;
				group->objectFile = fmt::format("{}/{}_{}.obj", objDir, file, moduleId);
				group->dependencyFile = fmt::format("{}/{}_{}.module.json", modulesDir, file, moduleId);
				group->otherFile = fmt::format("{}/{}_{}.ifc", modulesDir, file, moduleId);

				headerUnitList.emplace_back(std::move(group));
			}

			target.list = getModuleCommands(*inToolchain, headerUnitList, ModuleFileType::HeaderUnitDependency);

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
		}

		// Header units compiled first
		target.list.clear();
		std::vector<Unique<CommandPool::Target>> buildPortions;
		{
			SourceFileGroupList headerCompiles;

			for (const auto& header : headerUnits)
			{
				auto file = String::getPathFilename(header);

				auto group = std::make_unique<chalet::SourceFileGroup>();
				group->type = SourceType::CPlusPlus;
				group->sourceFile = header;
				group->objectFile = fmt::format("{}/{}_{}.obj", objDir, file, moduleId);
				group->dependencyFile = fmt::format("{}/{}_{}.ifc.d.json", modulesDir, file, moduleId);
				group->otherFile = fmt::format("{}/{}_{}.ifc", modulesDir, file, moduleId);

				headerCompiles.emplace_back(std::move(group));
			}

			auto portion = std::make_unique<CommandPool::Target>();
			portion->list = getModuleCommands(*inToolchain, headerCompiles, ModuleFileType::HeaderUnitObject);
			if (!portion->list.empty())
			{
				buildPortions.emplace_back(std::move(portion));
			}
		}

		{
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

				sourceCompiles.emplace_back(std::move(group));
			}

			auto portion = std::make_unique<CommandPool::Target>();
			portion->list = getModuleCommands(*inToolchain, sourceCompiles, ModuleFileType::ModuleObject);
			if (!portion->list.empty())
			{
				buildPortions.emplace_back(std::move(portion));
			}
		}

		//

		if (!buildPortions.empty())
		{
			addCompileCommands(buildPortions.front()->list, *inToolchain, inOutputs.groups);
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

			if (!buildPortions.empty())
			{
				buildPortions.back()->post = getLinkCommand(*inToolchain, inProject, inOutputs);

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
				portion->post = getLinkCommand(*inToolchain, inProject, inOutputs);
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
	}

	// Build in groups after dependencies / order have been resolved

	m_state.toolchain.setStrategy(m_oldStrategy);
	UNUSED(inProject, inToolchain);
	return true;
}

/*****************************************************************************/
CommandPool::CmdList IModuleStrategy::getModuleCommands(CompileToolchainController& inToolchain, const SourceFileGroupList& inGroups, const ModuleFileType inType)
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

			out.command = inToolchain.compilerCxx->getModuleCommand(source, target, dependency, interfaceFile, inType);

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

			bool sourceChanged = m_compileCache[source] || sourceCache.fileChangedOrDoesNotExist(source, target) || m_compileCache[source];
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
CommandPool::Cmd IModuleStrategy::getLinkCommand(CompileToolchainController& inToolchain, const SourceTarget& inProject, const SourceOutputs& inOutputs)
{
	const auto targetBasename = m_state.paths.getTargetBasename(inProject);

	CommandPool::Cmd ret;
	ret.command = inToolchain.getOutputTargetCommand(inOutputs.target, inOutputs.objectListLinker, targetBasename);
	ret.label = inProject.isStaticLibrary() ? "Archiving" : "Linking";
	ret.output = inOutputs.target;

	return ret;
}

}
