/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/ModuleStrategy/ModuleStrategyGCC.hpp"

#include "BuildEnvironment/IBuildEnvironment.hpp"
#include "Core/CommandLineInputs.hpp"
#include "Process/Environment.hpp"
#include "State/BuildPaths.hpp"
#include "State/BuildState.hpp"
#include "State/CompilerTools.hpp"
#include "System/Files.hpp"
#include "Utility/List.hpp"
#include "Utility/Path.hpp"
#include "Utility/String.hpp"
#include "Json/JsonComments.hpp"
#include "Json/JsonKeys.hpp"

namespace chalet
{
/*****************************************************************************/
ModuleStrategyGCC::ModuleStrategyGCC(BuildState& inState, CompileCommandsGenerator& inCompileCommandsGenerator) :
	IModuleStrategy(inState, inCompileCommandsGenerator)
{
}

/*****************************************************************************/
bool ModuleStrategyGCC::initialize()
{
	return IModuleStrategy::initialize();
}

/*****************************************************************************/
bool ModuleStrategyGCC::scanSourcesForModuleDependencies(CommandPool::Job& outJob)
{
	UNUSED(outJob);

	// We need to call this to update the compiler cache, but we don't want to use the commands
	auto commands = getModuleCommands(outputs->groups, Dictionary<ModulePayload>{}, ModuleFileType::ModuleDependency);
	UNUSED(commands);

	const std::string kModulePrefix("export module ");
	const std::string kImportPrefix("import ");
	const std::string kExportImportPrefix("export import ");
	const StringList kBreakOn{
		"namespace",
		"const",
		"using",
		"class",
		"struct"
	};

	auto readImports = [this](const std::string& line, const std::string& moduleName, const std::string& source, const std::string& prefix) {
		auto imported = line.substr(prefix.size(), line.find_last_of(';') - prefix.size());
		const bool hasAngleBrackets = imported.front() == '<' && imported.back() == '>';
		const bool hasQuotes = imported.front() == '"' && imported.back() == '"';
		if (hasAngleBrackets || hasQuotes)
		{
			imported = imported.substr(1, imported.size() - 2);

			if (m_headerUnitImports.find(source) == m_headerUnitImports.end())
				m_headerUnitImports.emplace(source, StringList{});

			// m_userHeaders.emplace_back(imported);

			for (auto& systemDir : m_systemHeaderDirectories)
			{
				auto resolved = fmt::format("{}/{}", systemDir, imported);
				if (Files::pathExists(resolved))
				{
					imported = std::move(resolved);
					break;
				}
			}

			m_headerUnitImports.at(source).emplace_back(std::move(imported));
		}
		else if (imported.front() == ':')
		{
			if (!moduleName.empty())
			{
				imported = fmt::format("{}{}", moduleName, imported);

				if (m_moduleImports.find(source) == m_moduleImports.end())
					m_moduleImports.emplace(source, StringList{});

				m_moduleImports.at(source).emplace_back(std::move(imported));
			}
		}
		else
		{
			if (m_moduleImports.find(source) == m_moduleImports.end())
				m_moduleImports.emplace(source, StringList{});

			m_moduleImports.at(source).emplace_back(std::move(imported));
		}
	};

	for (auto& group : outputs->groups)
	{
		if (group->type != SourceType::CPlusPlus)
			continue;

		auto& source = group->sourceFile;
		auto name = String::getPathBaseName(source);
		std::string moduleName;

		std::ifstream input(source);
		for (std::string line; std::getline(input, line);)
		{
			if (line.empty())
				continue;

			if (String::startsWith(kModulePrefix, line))
			{
				moduleName = line.substr(kModulePrefix.size(), line.find_last_of(';') - kModulePrefix.size());
				continue;
			}
			if (String::startsWith(kImportPrefix, line))
			{
				readImports(line, moduleName, source, kImportPrefix);
				continue;
			}
			if (String::startsWith(kExportImportPrefix, line))
			{
				readImports(line, moduleName, source, kExportImportPrefix);
				continue;
			}

			if (String::startsWith(kBreakOn, line))
				break;
		}

		if (moduleName.empty())
		{
			moduleName = source;
			m_modules[moduleName].implementationUnit = true;
		}

		if (!moduleName.empty())
		{
			m_reverseModuleLookup.emplace(source, moduleName);
		}
	}

	return true;
}

/*****************************************************************************/
bool ModuleStrategyGCC::readModuleDependencies()
{
	StringList foundSystemModules;
	auto kSystemModules = getSystemModules();

	for (auto& group : outputs->groups)
	{
		if (group->type != SourceType::CPlusPlus)
			continue;

		if (m_reverseModuleLookup.find(group->sourceFile) == m_reverseModuleLookup.end())
			continue;

		auto& name = m_reverseModuleLookup.at(group->sourceFile);

		m_modules[name].source = group->sourceFile;
		if (m_moduleImports.find(group->sourceFile) != m_moduleImports.end())
		{
			auto& modulesForSource = m_moduleImports.at(group->sourceFile);
			for (auto& moduleName : modulesForSource)
			{
				if (kSystemModules.find(moduleName) != kSystemModules.end())
				{
					List::addIfDoesNotExist(foundSystemModules, moduleName);
				}

				List::addIfDoesNotExist(m_modules[name].importedModules, moduleName);
			}
		}

		if (m_headerUnitImports.find(group->sourceFile) != m_headerUnitImports.end())
		{
			m_modules[name].importedHeaderUnits = m_headerUnitImports.at(group->sourceFile);
		}
	}

	if (!kSystemModules.empty())
	{
		/*
		u32 versionMajorMinor = m_state.toolchain.compilerCpp().versionMajorMinor;
		if (versionMajorMinor < 1400) // maybe?
		{
			// Move warning here later
		}
		*/

		for (auto& systemModule : foundSystemModules)
		{
			// Just show a warning for now
			Diagnostic::warn("'import {};' was used by a module, but C++23 Standard libary modules aren't supported by this compiler.", systemModule);

			/*
			if (kSystemModules.find(systemModule) != kSystemModules.end())
			{
				auto& filename = kSystemModules.at(systemModule);
				auto resolvedPath = fmt::format("{}/modules/{}", m_systemHeaderDirectory, filename);
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
			*/
		}
	}

	return true;
}

/*****************************************************************************/
bool ModuleStrategyGCC::readIncludesFromDependencyFile(const std::string& inFile, StringList& outList)
{
	UNUSED(inFile, outList);

	// TODO - the module dependency files are kind of incomplete when you use import <MyHeader.hpp>

	return true;
}

/*****************************************************************************/
bool ModuleStrategyGCC::scanHeaderUnitsForModuleDependencies(CommandPool::Job& outJob)
{
	UNUSED(outJob);

	// We need to call this to update the compiler cache, but we don't want to use the commands
	auto commands = getModuleCommands(m_headerUnitList, m_modulePayload, ModuleFileType::HeaderUnitDependency);
	UNUSED(commands);

	Dictionary<std::string> mapFiles;

	auto& cwd = m_state.inputs.workingDirectory();

	for (auto& [module, payload] : m_modulePayload)
	{
		std::string moduleContents;
		for (auto& headerMap : payload.headerUnitTranslations)
		{
			auto split = String::split(headerMap, '=');
			auto file = Files::getCanonicalPath(split[0]);

			// This is only needed by header-units
			String::replaceAll(file, cwd, ".");

			moduleContents += fmt::format("{} {}\n", file, split[1]);

			auto& name = split[0];
			if (mapFiles.find(name) == mapFiles.end())
			{
				mapFiles.emplace(name, fmt::format("{} {}\n", file, split[1]));
			}
		}

		for (auto& moduleMap : payload.moduleTranslations)
		{
			auto split = String::split(moduleMap, '=');
			moduleContents += fmt::format("{} {}\n", split[0], split[1]);
		}

		if (mapFiles.find(module) == mapFiles.end())
		{
			if (m_reverseModuleLookup.find(module) != m_reverseModuleLookup.end())
			{
				auto& moduleName = m_reverseModuleLookup.at(module);
				if (!m_modules[moduleName].implementationUnit)
				{
					auto modulePath = m_state.environment->getModuleBinaryInterfaceFile(module);
					moduleContents += fmt::format("{} {}\n", moduleName, modulePath);
				}
			}
			mapFiles.emplace(module, std::move(moduleContents));
		}
	}

	for (auto& [name, contents] : mapFiles)
	{
		std::string mapFile;
		if (isSystemHeaderFileOrModuleFile(name))
			mapFile = String::getPathFilename(name);
		else
			mapFile = name;

		auto outputFile = m_state.environment->getModuleDirectivesDependencyFile(mapFile);
		// if (!Files::pathExists(outputFile))
		{
			Files::createFileWithContents(outputFile, contents, true);
		}
	}

	return true;
}

/*****************************************************************************/
Dictionary<std::string> ModuleStrategyGCC::getSystemModules() const
{
	Dictionary<std::string> ret;

	ret.emplace("std", "std");
	ret.emplace("std.compat", "std.compat");

	return ret;
}

}
