/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Dependencies/DependencyManager.hpp"

#include "Core/CommandLineInputs.hpp"
#include "Dependencies/GitRunner.hpp"
#include "State/AncillaryTools.hpp"
#include "State/Dependency/GitDependency.hpp"
#include "State/StatePrototype.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Output.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"
#include "Json/JsonComments.hpp"

// TODO: Implement ThreadPool

namespace chalet
{
/*****************************************************************************/
DependencyManager::DependencyManager(const CommandLineInputs& inInputs, StatePrototype& inPrototype) :
	m_inputs(inInputs),
	m_prototype(inPrototype)
{
}

/*****************************************************************************/
bool DependencyManager::run()
{
	m_prototype.cache.file().loadExternalDependencies(m_inputs.externalDirectory());

	Output::lineBreak();

	m_fetched = false;

	for (auto& dependency : m_prototype.externalDependencies)
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

	if (!m_fetched)
		Output::previousLine();

	m_prototype.cache.file().saveExternalDependencies();

	return true;
}

/*****************************************************************************/
bool DependencyManager::runGitDependency(const GitDependency& inDependency)
{
	m_destinationCache.push_back(inDependency.destination());

	if (m_prototype.tools.git().empty())
		return true;

	GitRunner git(m_prototype, inDependency);
	if (!git.run(m_inputs.command() != Route::Configure))
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

	auto& dependencyCache = m_prototype.cache.file().externalDependencies();
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
	auto& dependencyCache = m_prototype.cache.file().externalDependencies();

	const auto& externalDir = m_inputs.externalDirectory();
	for (auto& it : inList)
	{
		if (Commands::pathExists(it))
		{
			if (Commands::removeRecursively(it))
			{
				std::string name = it;
				String::replaceAll(name, fmt::format("{}/", externalDir), "");

				Output::msgDisplayBlack(fmt::format("Removed unused dependency: '{}'", name));
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
	const auto& externalDir = m_inputs.externalDirectory();
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
