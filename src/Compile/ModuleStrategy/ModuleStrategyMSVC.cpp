/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/ModuleStrategy/ModuleStrategyMSVC.hpp"

#include "BuildEnvironment/IBuildEnvironment.hpp"
#include "Process/Environment.hpp"
#include "State/BuildInfo.hpp"
#include "State/BuildPaths.hpp"
#include "State/BuildState.hpp"
#include "State/CompilerTools.hpp"
#include "System/Files.hpp"
#include "Terminal/Output.hpp"
#include "Utility/List.hpp"
#include "Utility/Path.hpp"
#include "Utility/String.hpp"
#include "Json/JsonComments.hpp"
#include "Json/JsonKeys.hpp"

namespace chalet
{
/*****************************************************************************/
ModuleStrategyMSVC::ModuleStrategyMSVC(BuildState& inState, CompileCommandsGenerator& inCompileCommandsGenerator) :
	IModuleStrategy(inState, inCompileCommandsGenerator)
{
}

/*****************************************************************************/
bool ModuleStrategyMSVC::initialize()
{
	if (!IModuleStrategy::initialize())
		return false;

	if (!m_systemHeaderDirectories.empty())
	{
		m_systemModuleDirectory = fmt::format("{}/modules", m_systemHeaderDirectories.front());
	}

	return true;
}

/*****************************************************************************/
bool ModuleStrategyMSVC::scanSourcesForModuleDependencies(CommandPool::Job& outJob)
{
	// Scan sources for module dependencies

	outJob.list = getModuleCommands(outputs->groups, Dictionary<ModulePayload>{}, ModuleFileType::ModuleDependency);
	if (!outJob.list.empty())
	{
		// Output::msgScanningForModuleDependencies();
		// Output::lineBreak();

		CommandPool commandPool(m_state.info.maxJobs());
		if (!commandPool.run(outJob, m_compileAdapter.getCommandPoolSettings()))
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
bool ModuleStrategyMSVC::readModuleDependencies()
{
	// Version 1.1

	StringList foundSystemModules;
	auto kSystemModules = getSystemModules();

	for (auto& group : outputs->groups)
	{
		if (group->type != SourceType::CPlusPlus)
			continue;

		if (!Files::pathExists(group->dependencyFile))
			continue;

		Json jRoot;
		if (!JsonComments::parse(jRoot, group->dependencyFile))
		{
			Diagnostic::error("Failed to parse: {}", group->dependencyFile);
			return false;
		}

		const auto version = json::get<std::string>(jRoot, MSVCKeys::Version);
		if (version.empty())
		{
			Diagnostic::error("{}: Missing expected key '{}'", group->dependencyFile, MSVCKeys::Version);
			return false;
		}

		if (!String::equals("1.1", version))
		{
			Diagnostic::error("{}: Found version '{}', but only '1.1' is supported", group->dependencyFile, version);
			return false;
		}

		if (!json::isObject(jRoot, MSVCKeys::Data))
		{
			Diagnostic::error("{}: Missing expected key '{}'", group->dependencyFile, MSVCKeys::Data);
			return false;
		}

		const auto& data = jRoot.at(MSVCKeys::Data);
		if (!json::isValid<std::string>(data, MSVCKeys::ProvidedModule))
		{
			Diagnostic::error("{}: Missing expected key '{}'", group->dependencyFile, MSVCKeys::ProvidedModule);
			return false;
		}

		if (!json::isArray(data, MSVCKeys::ImportedModules))
		{
			Diagnostic::error("{}: Missing expected key '{}'", group->dependencyFile, MSVCKeys::ImportedModules);
			return false;
		}

		if (!json::isArray(data, MSVCKeys::ImportedHeaderUnits))
		{
			Diagnostic::error("{}: Missing expected key '{}'", group->dependencyFile, MSVCKeys::ImportedHeaderUnits);
			return false;
		}

		auto name = data.at(MSVCKeys::ProvidedModule).get<std::string>();
		if (name.empty())
		{
			m_modules[name].implementationUnit = true;
		}

		m_modules[name].source = group->sourceFile;

		const auto& importedModules = data.at(MSVCKeys::ImportedModules);
		for (auto& mod : importedModules)
		{
			if (!mod.is_string())
			{
				Diagnostic::error("{}: Unexpected structure for '{}'", group->dependencyFile, MSVCKeys::ImportedModules);
				return false;
			}

			auto moduleName = mod.get<std::string>();
			if (kSystemModules.find(moduleName) != kSystemModules.end())
			{
				List::addIfDoesNotExist(foundSystemModules, moduleName);
			}
			List::addIfDoesNotExist(m_modules[name].importedModules, std::move(moduleName));
		}

		const auto& importedHeaderUnits = data.at(MSVCKeys::ImportedHeaderUnits);
		for (auto& file : importedHeaderUnits)
		{
			if (!file.is_string())
			{
				Diagnostic::error("{}: Unexpected structure for '{}'", group->dependencyFile, MSVCKeys::ImportedHeaderUnits);
				return false;
			}

			auto outHeader = file.get<std::string>();
			Path::toUnix(outHeader);

			List::addIfDoesNotExist(m_modules[name].importedHeaderUnits, std::move(outHeader));
		}
	}

	if (!kSystemModules.empty())
	{
		for (auto& systemModule : foundSystemModules)
		{
			if (kSystemModules.find(systemModule) != kSystemModules.end())
			{
				auto& filename = kSystemModules.at(systemModule);
				auto resolvedPath = fmt::format("{}/{}", m_systemModuleDirectory, filename);
				if (Files::pathExists(resolvedPath))
				{
					m_modules[systemModule].source = std::move(resolvedPath);
					m_modules[systemModule].systemModule = true;

					if (String::equals("std.compat", systemModule))
					{
						// This is a bit of a hack so we don't have to scan std and std.compat deps if they're not used
						// maybe fix later...
						//
						m_modules[systemModule].importedModules.push_back("std");
					}
				}
			}
		}
	}

	return true;
}

/*****************************************************************************/
bool ModuleStrategyMSVC::readIncludesFromDependencyFile(const std::string& inFile, StringList& outList)
{
	Json jRoot;
	if (!JsonComments::parse(jRoot, inFile))
	{
		Diagnostic::error("Failed to parse: {}", inFile);
		return false;
	}

	if (!json::isValid<std::string>(jRoot, MSVCKeys::Version))
	{
		Diagnostic::error("{}: Missing expected key '{}'", inFile, MSVCKeys::Version);
		return false;
	}

	const auto version = json::get<std::string>(jRoot, MSVCKeys::Version);
	if (!String::equals("1.2", version))
	{
		Diagnostic::error("{}: Found version '{}', but only '1.2' is supported", inFile, version);
		return false;
	}

	if (!json::isObject(jRoot, MSVCKeys::Data))
	{
		Diagnostic::error("{}: Missing expected key '{}'", inFile, MSVCKeys::Data);
		return false;
	}

	const auto& data = jRoot.at(MSVCKeys::Data);
	if (!json::isArray(data, MSVCKeys::Includes))
	{
		Diagnostic::error("{}: Missing expected key '{}'", inFile, MSVCKeys::Includes);
		return false;
	}

	const auto& includes = data.at(MSVCKeys::Includes);
	for (auto& include : includes)
	{
		if (!include.is_string())
		{
			Diagnostic::error("{}: Unexpected structure for '{}'", inFile, MSVCKeys::Includes);
			return false;
		}

		auto outInclude = include.get<std::string>();
		Path::toUnix(outInclude);

		outList.emplace_back(std::move(outInclude));
	}

	return true;
}

/*****************************************************************************/
bool ModuleStrategyMSVC::scanHeaderUnitsForModuleDependencies(CommandPool::Job& outJob)
{
	outJob.list = getModuleCommands(m_headerUnitList, m_modulePayload, ModuleFileType::HeaderUnitDependency);
	if (!outJob.list.empty())
	{
		// Scan sources for module dependencies

		// Output::msgBuildingRequiredHeaderUnits();
		// Output::lineBreak();

		CommandPool commandPool(m_state.info.maxJobs());
		if (!commandPool.run(outJob, m_compileAdapter.getCommandPoolSettings()))
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
Dictionary<std::string> ModuleStrategyMSVC::getSystemModules() const
{
	Dictionary<std::string> ret;

	if (!m_systemModuleDirectory.empty())
	{
		auto modulesJsonPath = fmt::format("{}/modules.json", m_systemModuleDirectory);
		if (!Files::pathExists(modulesJsonPath))
			return ret;

		Json jRoot;
		if (!JsonComments::parse(jRoot, modulesJsonPath))
			return ret;

		if (jRoot.contains("module-sources"))
		{
			const auto& sources = jRoot.at("module-sources");
			if (sources.is_array())
			{
				for (auto& value : sources)
				{
					if (value.is_string())
					{
						auto filename = value.get<std::string>();
						auto moduleName = String::getPathFolderBaseName(filename);
						ret.emplace(moduleName, std::move(filename));
					}
				}
			}
		}
	}

	return ret;
}

}
