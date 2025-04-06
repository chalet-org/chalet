/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Dependencies/DependencyManager.hpp"

#include "Builder/ScriptRunner.hpp"
#include "Core/CommandLineInputs.hpp"
#include "Dependencies/ArchiveDependencyBuilder.hpp"
#include "Dependencies/GitDependencyBuilder.hpp"
#include "Dependencies/IDependencyBuilder.hpp"
#include "State/AncillaryTools.hpp"
#include "State/CentralState.hpp"
#include "State/Dependency/ArchiveDependency.hpp"
#include "State/Dependency/GitDependency.hpp"
#include "State/Dependency/ScriptDependency.hpp"
#include "System/Files.hpp"
#include "Terminal/Output.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"
#include "Json/JsonComments.hpp"

// TODO: Implement ThreadPool

namespace chalet
{
/*****************************************************************************/
DependencyManager::DependencyManager(CentralState& inCentralState) :
	m_centralState(inCentralState)
{
}

/*****************************************************************************/
bool DependencyManager::run()
{
	m_centralState.cache.file().loadExternalDependencies(m_centralState.inputs().externalDirectory());

	m_depsChanged.clear();

	for (auto& dependency : m_centralState.externalDependencies)
	{
		if (!dependency->validate())
		{
			Diagnostic::error("Error validating the '{}' dependency.", dependency->name());
			return false;
		}

		if (dependency->isGit() || dependency->isArchive())
		{
			auto builder = IDependencyBuilder::make(m_centralState, *dependency);
			if (!builder->validateRequiredTools())
				return false;

			if (!builder->resolveDependency(m_depsChanged))
				return false;
		}
		else if (dependency->isScript())
		{
			if (!runScriptDependency(static_cast<ScriptDependency&>(*dependency)))
				return false;
		}
	}

	StringList eraseList = getUnusedDependencies();
	if (!removeUnusedDependencies(eraseList))
		return false;

	if (!removeExternalDependencyDirectoryIfEmpty())
		return false;

	m_centralState.cache.file().saveExternalDependencies();

	return true;
}

/*****************************************************************************/
bool DependencyManager::runScriptDependency(const ScriptDependency& inDependency)
{
	const auto& file = inDependency.file();
	if (file.empty())
		return false;

	bool hasChanged = false;
	for (auto& dep : m_depsChanged)
	{
		if (String::startsWith(dep, file))
		{
			hasChanged = true;
			break;
		}
	}

	auto cachedName = fmt::format("{}/{}", m_centralState.inputs().externalDirectory(), inDependency.name());
	auto& dependencyCache = m_centralState.cache.file().externalDependencies();
	if (dependencyCache.contains(cachedName))
	{
		Json jRoot = dependencyCache.get(cachedName);
		if (!jRoot.is_object())
			jRoot = Json::object();

		auto args = String::join(inDependency.arguments());

		const auto lastCachedFile = json::get<std::string>(jRoot, "f");
		const auto lastCachedArgs = json::get<std::string>(jRoot, "a");

		bool fileNeedsUpdate = lastCachedFile != file;
		bool argsNeedsUpdate = lastCachedArgs != args;

		if (!hasChanged && !fileNeedsUpdate && !argsNeedsUpdate)
			return true;
	}
	else
	{
		Json jRoot;
		jRoot["f"] = file;
		jRoot["a"] = String::join(inDependency.arguments());

		dependencyCache.emplace(cachedName, std::move(jRoot));
	}

	const auto& arguments = inDependency.arguments();
	const auto& cwd = inDependency.workingDirectory();
	ScriptRunner scriptRunner(m_centralState.inputs(), m_centralState.tools);
	bool showExitCode = false;
	if (!scriptRunner.run(inDependency.scriptType(), file, arguments, cwd, showExitCode))
		return false;

	return true;
}

/*****************************************************************************/
StringList DependencyManager::getUnusedDependencies() const
{
	StringList destinationCache;
	for (auto& dependency : m_centralState.externalDependencies)
	{
		if (dependency->isGit())
		{
			auto& gitDependency = static_cast<const GitDependency&>(*dependency);
			destinationCache.push_back(gitDependency.destination());
		}
		else if (dependency->isArchive())
		{
			auto& archiveDependency = static_cast<const ArchiveDependency&>(*dependency);
			destinationCache.push_back(archiveDependency.destination());
		}
		else if (dependency->isScript())
		{
			destinationCache.push_back(fmt::format("{}/{}", m_centralState.inputs().externalDirectory(), dependency->name()));
		}
	}

	auto& dependencyCache = m_centralState.cache.file().externalDependencies();
	return dependencyCache.getKeys([&destinationCache](const std::string key) {
		return !List::contains(destinationCache, key);
	});
}

/*****************************************************************************/
bool DependencyManager::removeUnusedDependencies(const StringList& inList)
{
	const auto& externalDir = m_centralState.inputs().externalDirectory();
	auto& dependencyCache = m_centralState.cache.file().externalDependencies();

	for (auto& it : inList)
	{
		if (Files::pathExists(it))
		{
			if (Files::removeRecursively(it))
			{
				std::string name = it;
				String::replaceAll(name, fmt::format("{}/", externalDir), "");

				Output::msgRemovedUnusedDependency(name);
			}
		}

		dependencyCache.erase(it);
	}

	return true;
}

/*****************************************************************************/
bool DependencyManager::removeExternalDependencyDirectoryIfEmpty() const
{
	const auto& externalDir = m_centralState.inputs().externalDirectory();
	if (Files::pathIsEmpty(externalDir))
	{
		if (!Files::removeIfExists(externalDir))
		{
			Diagnostic::error("Error removing folder: {}", externalDir);
			return false;
		}
	}

	return true;
}
}
