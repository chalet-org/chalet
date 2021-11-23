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
	}

	return true;
}

/*****************************************************************************/
bool ModuleStrategyMSVC::readModuleDependencies(const SourceOutputs& inOutputs, Dictionary<ModuleLookup>& outModules)
{
	// Version 1.1

	const std::string kKeyVersion{ "Version" };
	const std::string kKeyData{ "Data" };
	const std::string kKeyProvidedModule{ "ProvidedModule" };
	const std::string kKeyImportedModules{ "ImportedModules" };
	const std::string kKeyImportedHeaderUnits{ "ImportedHeaderUnits" };

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

		if (!json.contains(kKeyVersion) || !json.at(kKeyVersion).is_string())
		{
			Diagnostic::error("{}: Missing expected key '{}'", group->dependencyFile, kKeyVersion);
			return false;
		}

		std::string version = json.at(kKeyVersion).get<std::string>();
		if (!String::equals("1.1", version))
		{
			Diagnostic::error("{}: Found version '{}', but only '1.1' is supported", group->dependencyFile, version);
			return false;
		}

		if (!json.contains(kKeyData) || !json.at(kKeyData).is_object())
		{
			Diagnostic::error("{}: Missing expected key '{}'", group->dependencyFile, kKeyData);
			return false;
		}

		const auto& data = json.at(kKeyData);
		if (!data.contains(kKeyProvidedModule) || !data.at(kKeyProvidedModule).is_string())
		{
			Diagnostic::error("{}: Missing expected key '{}'", group->dependencyFile, kKeyProvidedModule);
			return false;
		}

		if (!data.contains(kKeyImportedModules) || !data.at(kKeyImportedModules).is_array())
		{
			Diagnostic::error("{}: Missing expected key '{}'", group->dependencyFile, kKeyImportedModules);
			return false;
		}

		if (!data.contains(kKeyImportedHeaderUnits) || !data.at(kKeyImportedHeaderUnits).is_array())
		{
			Diagnostic::error("{}: Missing expected key '{}'", group->dependencyFile, kKeyImportedHeaderUnits);
			return false;
		}

		auto name = data.at(kKeyProvidedModule).get<std::string>();
		if (name.empty())
			name = kRootModule;

		outModules[name].source = group->sourceFile;

		for (auto& moduleItr : data.at(kKeyImportedModules).items())
		{
			auto& mod = moduleItr.value();
			if (!mod.is_string())
			{
				Diagnostic::error("{}: Unexpected structure for '{}'", group->dependencyFile, kKeyImportedModules);
				return false;
			}

			List::addIfDoesNotExist(outModules[name].importedModules, mod.get<std::string>());
		}

		for (auto& fileItr : data.at(kKeyImportedHeaderUnits).items())
		{
			auto& file = fileItr.value();
			if (!file.is_string())
			{
				Diagnostic::error("{}: Unexpected structure for '{}'", group->dependencyFile, kKeyImportedHeaderUnits);
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
