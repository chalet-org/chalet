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

	m_fetched = false;

	for (auto& dependency : m_centralState.externalDependencies)
	{
		if (dependency->isGit())
		{
			if (!runGitDependency(static_cast<const GitDependency&>(*dependency)))
				return false;
		}
	}

	StringList eraseList = getUnusedDependencies();
	if (!removeUnusedDependencies(eraseList))
		return false;

	if (!removeExternalDependencyDirectoryIfEmpty())
		return false;

	// if (!m_fetched)
	// 	Output::previousLine();

	m_centralState.cache.file().saveExternalDependencies();

	return true;
}

/*****************************************************************************/
bool DependencyManager::runGitDependency(const GitDependency& inDependency)
{
	m_destinationCache.push_back(inDependency.destination());

	if (m_centralState.tools.git().empty())
		return true;

	bool doNotUpdate = m_centralState.inputs().route() != Route::Configure;

	GitRunner git(m_centralState, inDependency);
	if (!git.run(doNotUpdate))
	{
		Diagnostic::error("Error fetching git dependency: {}", inDependency.name());
		return false;
	}

	m_fetched |= git.fetched();

	return true;
}

/*****************************************************************************/
StringList DependencyManager::getUnusedDependencies() const
{
	StringList ret;

	auto& dependencyCache = m_centralState.cache.file().externalDependencies();
	for (auto& [key, value] : dependencyCache)
	{
		if (!List::contains(m_destinationCache, key))
			ret.push_back(key);
	}

	return ret;
}

/*****************************************************************************/
bool DependencyManager::removeUnusedDependencies(const StringList& inList)
{
	auto& dependencyCache = m_centralState.cache.file().externalDependencies();

	const auto& externalDir = m_centralState.inputs().externalDirectory();
	for (auto& it : inList)
	{
		if (Commands::pathExists(it))
		{
			if (Commands::removeRecursively(it))
			{
				std::string name = it;
				String::replaceAll(name, fmt::format("{}/", externalDir), "");

				Output::msgRemovedUnusedDependency(name);
				m_fetched |= true;
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
	if (Commands::pathIsEmpty(externalDir, {}, true))
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
