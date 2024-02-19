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
	const auto& systemIncludeDir = m_state.toolchain.compilerCxxAny().includeDir;
	auto& version = m_state.toolchain.version();
	u32 versionMajor = m_state.toolchain.versionMajor();
	auto cppFolder = fmt::format("{}/c++", systemIncludeDir);
	if (Files::pathIsSymLink(cppFolder))
	{
		cppFolder = fmt::format("{}/{}", systemIncludeDir, Files::resolveSymlink(cppFolder));
	}

	auto versionFolder = Files::getCanonicalPath(fmt::format("{}/{}", cppFolder, versionMajor));
	if (!Files::pathExists(versionFolder))
	{
		versionFolder = Files::getCanonicalPath(fmt::format("{}/{}", cppFolder, version));
		if (!Files::pathExists(versionFolder))
		{
			Diagnostic::error("Could not resolve system include directory");
			return false;
		}
	}

	m_systemHeaderDirectory = std::move(versionFolder);

	return true;
}

/*****************************************************************************/
bool ModuleStrategyGCC::isSystemModuleFile(const std::string& inFile) const
{
	return !m_systemHeaderDirectory.empty() && String::startsWith(m_systemHeaderDirectory, inFile);
}

/*****************************************************************************/
std::string ModuleStrategyGCC::getBuildOutputForFile(const SourceFileGroup& inFile, const bool inIsObject) const
{
	std::string ret = inIsObject ? inFile.sourceFile : inFile.dependencyFile;
	if (String::startsWith(m_systemHeaderDirectory, ret))
	{
		ret = String::getPathFilename(ret);
	}

	return ret;
}

/*****************************************************************************/
bool ModuleStrategyGCC::scanSourcesForModuleDependencies(CommandPool::Job& outJob, CompileToolchainController& inToolchain, const SourceFileGroupList& inGroups)
{
	UNUSED(outJob, inToolchain);

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

			auto resolved = fmt::format("{}/{}", m_systemHeaderDirectory, imported);
			if (Files::pathExists(resolved))
				imported = std::move(resolved);

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

	for (auto& group : inGroups)
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
			moduleName = fmt::format("@{}", source);
		}

		if (!moduleName.empty())
		{
			m_moduleMap.emplace(source, moduleName);
		}
	}

	return true;
}

/*****************************************************************************/
bool ModuleStrategyGCC::scanHeaderUnitsForModuleDependencies(CommandPool::Job& outJob, CompileToolchainController& inToolchain, Dictionary<ModulePayload>& outPayload, const SourceFileGroupList& inGroups)
{
	UNUSED(outJob, inToolchain, inGroups);

	Dictionary<std::string> mapFiles;

	auto& cwd = m_state.inputs.workingDirectory();

	for (auto& [module, payload] : outPayload)
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
			if (m_moduleMap.find(module) != m_moduleMap.end())
			{
				auto& moduleName = m_moduleMap.at(module);
				if (!String::startsWith('@', moduleName))
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
		if (isSystemModuleFile(name))
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
bool ModuleStrategyGCC::readModuleDependencies(const SourceOutputs& inOutputs, Dictionary<ModuleLookup>& outModules)
{
	UNUSED(inOutputs);

	for (auto& group : inOutputs.groups)
	{
		if (group->type != SourceType::CPlusPlus)
			continue;

		if (m_moduleMap.find(group->sourceFile) == m_moduleMap.end())
			continue;

		auto& name = m_moduleMap.at(group->sourceFile);

		outModules[name].source = group->sourceFile;
		if (m_moduleImports.find(group->sourceFile) != m_moduleImports.end())
		{
			outModules[name].importedModules = m_moduleImports.at(group->sourceFile);
		}

		if (m_headerUnitImports.find(group->sourceFile) != m_headerUnitImports.end())
		{
			outModules[name].importedHeaderUnits = m_headerUnitImports.at(group->sourceFile);
		}
	}

	return true;
}

/*****************************************************************************/
bool ModuleStrategyGCC::readIncludesFromDependencyFile(const std::string& inFile, StringList& outList)
{
	UNUSED(inFile, outList);

	// TODO

	return true;
}

/*****************************************************************************/
Dictionary<std::string> ModuleStrategyGCC::getSystemModules() const
{
	Dictionary<std::string> ret;

	return ret;
}

}
