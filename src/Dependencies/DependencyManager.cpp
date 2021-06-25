/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Dependencies/DependencyManager.hpp"

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
DependencyManager::DependencyManager(StatePrototype& inPrototype) :
	m_prototype(inPrototype)
{
}

/*****************************************************************************/
bool DependencyManager::run(const bool inConfigureCmd)
{
	if (m_prototype.tools.git().empty())
		return true;

	const auto& externalDepDir = m_prototype.environment.externalDepDir();
	// auto& localSettings = m_prototype.cache.getSettings(SettingsType::Local);

	if (!m_prototype.externalDependencies.empty())
	{
		m_prototype.cache.file().loadExternalDependencies();

		Output::lineBreak();
	}

	auto& dependencyCache = m_prototype.cache.file().externalDependencies();

	StringList destinationCache;

	bool result = true;
	int count = 0;
	for (auto& dependency : m_prototype.externalDependencies)
	{
		auto& git = static_cast<const GitDependency&>(*dependency);

		const auto& repository = git.repository();
		const auto& destination = git.destination();

		destinationCache.push_back(destination);

		bool update = false;

		// During the build, just continue if the path already exists
		//   (the install command can be used for more complicated tasks)
		LOG(destination);
		if (Commands::pathExists(destination))
		{
			if (!dependencyCache.contains(destination))
			{
				auto commitHash = m_prototype.tools.getCurrentGitRepositoryHash(destination);
				dependencyCache.emplace(destination, std::move(commitHash));
			}
			if (!inConfigureCmd)
				continue;

			update = true;
		}

		const bool submodules = git.submodules();

		const auto& branch = git.branch();
		const auto& tag = git.tag();
		const auto& commit = git.commit();

		const bool branchValid = !branch.empty();
		const bool tagValid = !tag.empty();
		const bool commitValid = !commit.empty();

		bool res = true;

		if (update && branchValid)
		{
			const std::string currentBranch = m_prototype.tools.getCurrentGitRepositoryBranch(destination);
			LOG("branches", fmt::format("'{}'", currentBranch), fmt::format("'{}'", branch));

			// in this case, HEAD means the branch is on a tag or commit
			if ((currentBranch != "HEAD" || commitValid) && currentBranch != branch)
			{
				// we need to fetch a new branch (from a shallow clone), so it's easier to start fresh
				result &= Commands::removeRecursively(destination);
				update = false;
			}
		}

		if (update && commitValid)
		{
			const std::string currentCommit = m_prototype.tools.getCurrentGitRepositoryHash(destination);
			LOG("commits:", fmt::format("'{}'", currentCommit), fmt::format("'{}'", commit));

			if (!String::startsWith(commit, currentCommit))
			{
				// we need to fetch a new branch (from a shallow clone), so it's easier to start fresh
				result &= Commands::removeRecursively(destination);
				update = false;
			}
		}

		if (update && tagValid)
		{
			const std::string currentTag = m_prototype.tools.getCurrentGitRepositoryTag(destination);
			LOG("tags:", fmt::format("'{}'", currentTag), fmt::format("'{}'", tag));

			if (currentTag != tag)
			{
				// we need to fetch a new branch (from a shallow clone), so it's easier to start fresh
				result &= Commands::removeRecursively(destination);
				update = false;
			}
		}

		if (update)
		{
			LOG("update");
			// if commit or tag was specified, we shouldn't update this way
			if (commitValid || tagValid)
				continue;

			LOG(destination);
			if (dependencyCache.contains(destination))
			{
				// We're using a shallow clone, so this only works if the branch hasn't changed
				const std::string originHash = m_prototype.tools.getCurrentGitRepositoryHashFromRemote(destination, branch);
				const auto& cachedHash = dependencyCache.get(destination);

				if (cachedHash == originHash)
					continue;
			}

			const auto& checkoutTo = tagValid ? tag : branch;

			Output::msgUpdatingDependency(repository, checkoutTo);

			res &= m_prototype.tools.updateGitRepositoryShallow(destination);
			result &= res;
		}

		if (!update)
		{
			const auto& checkoutTo = tagValid ? tag : branch;

			Output::msgFetchingDependency(repository, checkoutTo);

			uint maxJobs = m_prototype.environment.maxJobs();

			StringList cmd;
			cmd.push_back(m_prototype.tools.git());
			cmd.push_back("clone");
			cmd.push_back("--quiet");

			if (!checkoutTo.empty())
			{
				cmd.push_back("-b");
				cmd.push_back(checkoutTo);
			}

			if (commitValid)
			{
				cmd.push_back("--single-branch");
			}
			else
			{
				cmd.push_back("--depth");
				cmd.push_back("1");
			}
			if (submodules)
			{
				cmd.push_back("--recurse-submodules");
				cmd.push_back("--shallow-submodules");
				cmd.push_back("--no-remote-submodules");
				cmd.push_back("-j");
				cmd.push_back(std::to_string(maxJobs));
			}
			cmd.push_back("--config");
			cmd.push_back("advice.detachedHead=false");
			cmd.push_back(repository);
			cmd.push_back(destination);

			// LOG(cmd);

			res &= Commands::subprocess(cmd);

			if (commitValid)
			{
				res &= m_prototype.tools.resetGitRepositoryToCommit(destination, commit);
			}

			result &= res;
		}

		if (res)
		{
			auto commitHash = m_prototype.tools.getCurrentGitRepositoryHash(destination);

			// Output::msgDisplayBlack(commitHash); // useful for debugging
			dependencyCache.set(destination, std::move(commitHash));
		}
		else
		{
			result &= Commands::removeRecursively(destination);
		}

		++count;
	}

	if (result)
	{
		StringList eraseList;
		for (auto& [key, value] : dependencyCache)
		{
			if (!List::contains(destinationCache, key))
				eraseList.push_back(key);
		}

		for (auto& it : eraseList)
		{
			if (Commands::pathExists(it))
			{
				const bool removed = Commands::removeRecursively(it);
				if (removed)
				{
					std::string name = it;
					String::replaceAll(name, fmt::format("{}/", externalDepDir), "");

					Output::msgDisplayBlack(fmt::format("Removed unused dependency: '{}'", name));
					++count;
				}

				result &= removed;
			}

			dependencyCache.erase(it);
		}

		if (Commands::pathIsEmpty(externalDepDir, {}, true))
			result &= Commands::remove(externalDepDir);
	}

	if (!m_prototype.externalDependencies.empty())
	{
		m_prototype.cache.file().saveExternalDependencies();
	}

	return result;
}
}
