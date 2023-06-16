/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/ModuleStrategy/ModuleStrategyMSVC.hpp"

#include "State/BuildPaths.hpp"
#include "State/BuildState.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Environment.hpp"
#include "Terminal/Path.hpp"
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
		m_msvcToolsDirectory = Environment::getAsString("VCToolsInstallDir");
		Path::sanitize(m_msvcToolsDirectory);

		m_msvcToolsDirectoryLower = String::toLowerCase(m_msvcToolsDirectory);
		Path::sanitizeForWindows(m_msvcToolsDirectoryLower);
	}

	return true;
}

/*****************************************************************************/
bool ModuleStrategyMSVC::isSystemHeader(const std::string& inHeader) const
{
	return !m_msvcToolsDirectory.empty() && String::startsWith(m_msvcToolsDirectory, inHeader);
}

/*****************************************************************************/
bool ModuleStrategyMSVC::readModuleDependencies(const SourceOutputs& inOutputs, Dictionary<ModuleLookup>& outModules)
{
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

			List::addIfDoesNotExist(outModules[name].importedModules, mod.get<std::string>());
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
			Path::sanitize(outHeader);

			List::addIfDoesNotExist(outModules[name].importedHeaderUnits, std::move(outHeader));
		}
	}

	return true;
}

/*****************************************************************************/
std::string ModuleStrategyMSVC::getBuildOutputForFile(const SourceFileGroup& inFile, const bool inIsObject)
{
	std::string ret = inIsObject ? inFile.sourceFile : inFile.dependencyFile;
	if (String::startsWith(m_msvcToolsDirectory, ret))
	{
		ret = String::getPathFilename(ret);
	}

	return ret;
}

}
