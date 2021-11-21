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
	target.list = getModuleDependencyCommands(*inToolchain, inOutputs.groups);
	// bool targetExists = Commands::pathExists(inOutputs.target);

	if (!target.list.empty())
	// if (!target.list.empty() || !targetExists)
	{
		/*if (m_sourcesChanged || !targetExists)
		{
			if (!List::contains(m_fileCache, inOutputs.target))
			{
				m_fileCache.push_back(inOutputs.target);

				target->post = getLinkCommand(inOutputs.target, inOutputs.objectListLinker);
			}
		}*/

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

			const auto& id = m_state.uniqueId();
			const auto& objDir = m_state.paths.objDir();
			const auto& modulesDir = m_state.paths.modulesDir();

			for (auto& header : headerUnits)
			{
				auto file = String::getPathFilename(header);

				auto group = std::make_unique<chalet::SourceFileGroup>();
				group->type = SourceType::CPlusPlus;
				group->sourceFile = std::move(header);
				group->objectFile = fmt::format("{}/{}_{}.obj", objDir, file, id);
				group->dependencyFile = fmt::format("{}/{}_{}.module.json", modulesDir, file, id);
				group->otherFile = fmt::format("{}/{}_{}.ifc", objDir, file, id);

				headerUnitList.emplace_back(std::move(group));
			}

			target.list = getModuleDependencyCommands(*inToolchain, headerUnitList);

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
		/*{
			SourceFileGroupList headerUnitList;

			const auto& id = m_state.uniqueId();
			const auto& objDir = m_state.paths.objDir();
			const auto& modulesDir = m_state.paths.modulesDir();

			for (auto& header : headerUnits)
			{
				auto file = String::getPathFilename(header);

				auto group = std::make_unique<chalet::SourceFileGroup>();
				group->type = SourceType::CPlusPlus;
				group->sourceFile = std::move(header);
				group->objectFile = fmt::format("{}/{}_{}.obj", objDir, file, id);
				group->dependencyFile = fmt::format("{}/{}_{}.module.json", modulesDir, file, id);
				group->otherFile = fmt::format("{}/{}_{}.ifc", objDir, file, id);

				headerUnitList.emplace_back(std::move(group));
			}

			target.list = getModuleDependencyCommands(*inToolchain, headerUnitList);

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
		}*/
	}

	// Build in groups after dependencies / order have been resolved

	m_state.toolchain.setStrategy(m_oldStrategy);
	UNUSED(inProject, inToolchain);
	return true;
}

/*****************************************************************************/
CommandPool::CmdList IModuleStrategy::getModuleDependencyCommands(CompileToolchainController& inToolchain, const SourceFileGroupList& inGroups)
{
	auto& sourceCache = m_state.cache.file().sources();

	CommandPool::CmdList ret;
	const auto& objDir = m_state.paths.objDir();

	for (auto& group : inGroups)
	{
		const auto& source = group->sourceFile;

		if (source.empty())
			continue;

		const auto& target = group->objectFile;
		const auto& dependency = group->dependencyFile;
		auto interfaceFile = group->otherFile;
		if (interfaceFile.empty())
		{
			interfaceFile = fmt::format("{}/{}.ifc", objDir, source);
		}

		if (group->type == SourceType::CPlusPlus)
		{
			bool sourceChanged = sourceCache.fileChangedOrDoesNotExist(source, dependency);
			m_sourcesChanged |= sourceChanged;
			if (sourceChanged)
			{
				CommandPool::Cmd out;
				out.output = dependency;
				out.command = inToolchain.compilerCxx->getModuleDependencyCommand(source, target, dependency, interfaceFile);

#if defined(CHALET_WIN32)
				if (m_state.environment->isMsvc())
					out.outputReplace = String::getPathFilename(source);
#endif

				ret.emplace_back(std::move(out));
			}
		}
	}

	return ret;
}

/*****************************************************************************/
CommandPool::CmdList IModuleStrategy::getCompileCommands(CompileToolchainController& inToolchain, const SourceFileGroupList& inGroups)
{
	auto& sourceCache = m_state.cache.file().sources();

	CommandPool::CmdList ret;

	for (auto& group : inGroups)
	{
		const auto& source = group->sourceFile;

		if (source.empty())
			continue;

		const auto& target = group->objectFile;
		const auto& dependency = group->dependencyFile;

		if (group->type == SourceType::WindowsResource)
		{
			bool sourceChanged = sourceCache.fileChangedOrDoesNotExist(source, target);
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

				ret.emplace_back(std::move(out));
			}
		}
		else if (group->type == SourceType::CPlusPlus)
		{
			CxxSpecialization specialization = CxxSpecialization::CPlusPlus;
			if (group->type == SourceType::ObjectiveC)
				specialization = CxxSpecialization::ObjectiveC;
			else if (group->type == SourceType::ObjectiveCPlusPlus)
				specialization = CxxSpecialization::ObjectiveCPlusPlus;

			bool sourceChanged = sourceCache.fileChangedOrDoesNotExist(source, target);
			m_sourcesChanged |= sourceChanged;
			if (sourceChanged)
			{
				CommandPool::Cmd out;
				out.output = source;
				out.command = inToolchain.compilerCxx->getCommand(source, target, false, dependency, specialization);

#if defined(CHALET_WIN32)
				if (m_state.environment->isMsvc())
					out.outputReplace = String::getPathFilename(out.output);
#endif

				ret.emplace_back(std::move(out));
			}
		}
	}

	return ret;
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
