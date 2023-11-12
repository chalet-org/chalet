/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/ModuleStrategy/ModuleStrategyMSVC.hpp"

#include "State/BuildPaths.hpp"
#include "State/BuildState.hpp"
#include "Terminal/Commands.hpp"
#include "Process/Environment.hpp"
#include "Utility/Path.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"
#include "Json/JsonComments.hpp"
#include "Json/JsonKeys.hpp"

namespace chalet
{
/*****************************************************************************/
ModuleStrategyMSVC::ModuleStrategyMSVC(BuildState& inState) :
	IModuleStrategy(inState)
{
}

/*****************************************************************************/
bool ModuleStrategyMSVC::initialize()
{
	if (m_msvcToolsDirectory.empty())
	{
		m_msvcToolsDirectory = Environment::getString("VCToolsInstallDir");
		Path::toUnix(m_msvcToolsDirectory);
	}

	return true;
}

/*****************************************************************************/
bool ModuleStrategyMSVC::isSystemModuleFile(const std::string& inFile) const
{
	return !m_msvcToolsDirectory.empty() && String::startsWith(m_msvcToolsDirectory, inFile);
}

/*****************************************************************************/
bool ModuleStrategyMSVC::readModuleDependencies(const SourceOutputs& inOutputs, Dictionary<ModuleLookup>& outModules)
{
	// Version 1.1

	auto kSystemModules = getSystemModules();
	// for (auto& [name, file] : kSystemModules)
	// {
	// 	LOG(name, file);
	// }

	StringList foundSystemModules;

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
			return false;
		}

		if (!json.contains(MSVCKeys::Version) || !json.at(MSVCKeys::Version).is_string())
		{
			Diagnostic::error("{}: Missing expected key '{}'", group->dependencyFile, MSVCKeys::Version);
			return false;
		}

		std::string version = json.at(MSVCKeys::Version).get<std::string>();
		if (!String::equals("1.1", version))
		{
			Diagnostic::error("{}: Found version '{}', but only '1.1' is supported", group->dependencyFile, version);
			return false;
		}

		if (!json.contains(MSVCKeys::Data) || !json.at(MSVCKeys::Data).is_object())
		{
			Diagnostic::error("{}: Missing expected key '{}'", group->dependencyFile, MSVCKeys::Data);
			return false;
		}

		const auto& data = json.at(MSVCKeys::Data);
		if (!data.contains(MSVCKeys::ProvidedModule) || !data.at(MSVCKeys::ProvidedModule).is_string())
		{
			Diagnostic::error("{}: Missing expected key '{}'", group->dependencyFile, MSVCKeys::ProvidedModule);
			return false;
		}

		if (!data.contains(MSVCKeys::ImportedModules) || !data.at(MSVCKeys::ImportedModules).is_array())
		{
			Diagnostic::error("{}: Missing expected key '{}'", group->dependencyFile, MSVCKeys::ImportedModules);
			return false;
		}

		if (!data.contains(MSVCKeys::ImportedHeaderUnits) || !data.at(MSVCKeys::ImportedHeaderUnits).is_array())
		{
			Diagnostic::error("{}: Missing expected key '{}'", group->dependencyFile, MSVCKeys::ImportedHeaderUnits);
			return false;
		}

		auto name = data.at(MSVCKeys::ProvidedModule).get<std::string>();
		if (name.empty())
			name = fmt::format("@{}", group->sourceFile);

		outModules[name].source = group->sourceFile;

		for (auto& moduleItr : data.at(MSVCKeys::ImportedModules).items())
		{
			auto& mod = moduleItr.value();
			if (!mod.is_string())
			{
				Diagnostic::error("{}: Unexpected structure for '{}'", group->dependencyFile, MSVCKeys::ImportedModules);
				return false;
			}

			auto moduleName = mod.get<std::string>();
			if (kSystemModules.find(moduleName) != kSystemModules.end())
			{
				foundSystemModules.emplace_back(moduleName);
			}
			List::addIfDoesNotExist(outModules[name].importedModules, std::move(moduleName));
		}

		for (auto& fileItr : data.at(MSVCKeys::ImportedHeaderUnits).items())
		{
			auto& file = fileItr.value();
			if (!file.is_string())
			{
				Diagnostic::error("{}: Unexpected structure for '{}'", group->dependencyFile, MSVCKeys::ImportedHeaderUnits);
				return false;
			}

			auto outHeader = file.get<std::string>();
			Path::toUnix(outHeader);

			List::addIfDoesNotExist(outModules[name].importedHeaderUnits, std::move(outHeader));
		}
	}

	if (!kSystemModules.empty())
	{
		for (auto& systemModule : foundSystemModules)
		{
			if (kSystemModules.find(systemModule) != kSystemModules.end())
			{
				auto& filename = kSystemModules.at(systemModule);
				auto resolvedPath = fmt::format("{}/modules/{}", m_msvcToolsDirectory, filename);
				if (Commands::pathExists(resolvedPath))
				{
					outModules[systemModule].source = std::move(resolvedPath);
					outModules[systemModule].systemModule = true;

					if (String::equals("std.compat", systemModule))
					{
						// This is a bit of a hack so we don't have to scan std and std.compat deps if they're not used
						// maybe fix later...
						//
						outModules[systemModule].importedModules.push_back("std");
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
	Json json;
	if (!JsonComments::parse(json, inFile))
	{
		Diagnostic::error("Failed to parse: {}", inFile);
		return false;
	}

	if (!json.contains(MSVCKeys::Version) || !json.at(MSVCKeys::Version).is_string())
	{
		Diagnostic::error("{}: Missing expected key '{}'", inFile, MSVCKeys::Version);
		return false;
	}

	std::string version = json.at(MSVCKeys::Version).get<std::string>();
	if (!String::equals("1.2", version))
	{
		Diagnostic::error("{}: Found version '{}', but only '1.2' is supported", inFile, version);
		return false;
	}

	if (!json.contains(MSVCKeys::Data) || !json.at(MSVCKeys::Data).is_object())
	{
		Diagnostic::error("{}: Missing expected key '{}'", inFile, MSVCKeys::Data);
		return false;
	}

	const auto& data = json.at(MSVCKeys::Data);
	if (!data.contains(MSVCKeys::Includes) || !data.at(MSVCKeys::Includes).is_array())
	{
		Diagnostic::error("{}: Missing expected key '{}'", inFile, MSVCKeys::Includes);
		return false;
	}

	for (auto& includeItr : data.at(MSVCKeys::Includes).items())
	{
		auto& include = includeItr.value();
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
std::string ModuleStrategyMSVC::getBuildOutputForFile(const SourceFileGroup& inFile, const bool inIsObject) const
{
	std::string ret = inIsObject ? inFile.sourceFile : inFile.dependencyFile;
	if (String::startsWith(m_msvcToolsDirectory, ret))
	{
		ret = String::getPathFilename(ret);
	}

	return ret;
}

/*****************************************************************************/
Dictionary<std::string> ModuleStrategyMSVC::getSystemModules() const
{
	Dictionary<std::string> ret;

	if (!m_msvcToolsDirectory.empty())
	{
		auto modulesJsonPath = fmt::format("{}/modules/modules.json", m_msvcToolsDirectory);
		if (!Commands::pathExists(modulesJsonPath))
			return ret;

		Json json;
		if (!JsonComments::parse(json, modulesJsonPath))
			return ret;

		if (json.contains("module-sources"))
		{
			const auto& sources = json.at("module-sources");
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
