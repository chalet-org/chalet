/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Dependencies/DependencyManager.hpp"

#include "Core/CommandLineInputs.hpp"
#include "Dependencies/GitRunner.hpp"
#include "State/AncillaryTools.hpp"
#include "State/CentralState.hpp"
#include "State/Dependency/GitDependency.hpp"
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

	// Output::lineBreak();

	if (!m_centralState.tools.git().empty())
	{
		GitRunner git(m_centralState);
		if (!git.run())
			return false;
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
