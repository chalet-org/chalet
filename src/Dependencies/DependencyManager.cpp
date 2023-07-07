/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Dependencies/DependencyManager.hpp"

#include "Builder/ScriptRunner.hpp"
#include "Core/CommandLineInputs.hpp"
#include "Dependencies/GitRunner.hpp"
#include "State/AncillaryTools.hpp"
#include "State/CentralState.hpp"
#include "State/Dependency/GitDependency.hpp"
#include "State/Dependency/ScriptDependency.hpp"
#include "Terminal/Commands.hpp"
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

	for (auto& dependency : m_centralState.externalDependencies)
	{
		if (!dependency->validate())
		{
			Diagnostic::error("Error validating the '{}' dependency.", dependency->name());
			return false;
		}

		if (dependency->isGit())
		{
			if (!m_centralState.tools.git().empty())
			{
				GitRunner git(m_centralState);
				if (!git.run(static_cast<GitDependency&>(*dependency)))
					return false;
			}
			else
			{
				Diagnostic::error("Git dependency '{}' requested, but git is not installed", dependency->name());
				return false;
			}
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

	auto cachedName = fmt::format("{}/{}", m_centralState.inputs().externalDirectory(), inDependency.name());
	auto& dependencyCache = m_centralState.cache.file().externalDependencies();
	if (dependencyCache.contains(cachedName))
	{
		Json json = dependencyCache.get(cachedName);
		if (!json.is_object())
			json = Json::object();

		auto args = String::join(inDependency.arguments());

		const auto lastCachedFile = json["f"].is_string() ? json["f"].get<std::string>() : std::string();
		const auto lastCachedArgs = json["a"].is_string() ? json["a"].get<std::string>() : std::string();

		bool fileNeedsUpdate = lastCachedFile != file;
		bool argsNeedsUpdate = lastCachedArgs != args;

		if (!fileNeedsUpdate && !argsNeedsUpdate)
			return true;
	}
	else
	{
		Json json;
		json["f"] = file;
		json["a"] = String::join(inDependency.arguments());

		dependencyCache.emplace(cachedName, std::move(json));
	}

	const auto& arguments = inDependency.arguments();
	ScriptRunner scriptRunner(m_centralState.inputs(), m_centralState.tools);
	bool showExitCode = false;
	if (!scriptRunner.run(inDependency.scriptType(), file, arguments, showExitCode))
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
		if (Commands::pathExists(it))
		{
			if (Commands::removeRecursively(it))
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
	if (Commands::pathIsEmpty(externalDir))
	{
		if (!Commands::remove(externalDir))
		{
			Diagnostic::error("Error removing folder: {}", externalDir);
			return false;
		}
	}

	return true;
}
}
