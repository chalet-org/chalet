/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Dependencies/DependencyManager.hpp"

#include "Libraries/Format.hpp"
#include "State/Dependency/GitDependency.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Output.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"
#include "Json/JsonComments.hpp"

// TODO: vcpkg, conan, buckaroo integration?
// TODO: Implement ThreadPool

namespace chalet
{
/*****************************************************************************/
DependencyManager::DependencyManager(BuildState& inState, const bool inCleanOutput) :
	m_state(inState),
	m_cleanOutput(inCleanOutput)
{
}

/*****************************************************************************/
bool DependencyManager::run(const bool inInstallCmd)
{
	if (m_state.tools.git().empty())
		return true;

	const auto& externalDepDir = m_state.paths.externalDepDir();
	auto& environmentCache = m_state.cache.environmentCache();

	Json& dependencyCache = environmentCache.json["externalDependencies"];

	StringList destinationCache;

	bool result = true;
	int count = 0;
	for (auto& dependency : m_state.externalDependencies)
	{
		auto& git = static_cast<const GitDependency&>(*dependency);

		const auto& repository = git.repository();
		const auto& destination = git.destination();

		if (destination.empty() || String::startsWith('.', destination) || String::startsWith('/', destination))
		{
			// This shouldn't occur, but would be pretty bad if it did
			Diagnostic::error(fmt::format("The external dependency destination was blank for '{}'.", repository));
			return false;
		}

		LOG(destination);

		destinationCache.push_back(destination);

		bool update = false;

		// During the build, just continue if the path already exists
		//   (the install command can be used for more complicated tasks)
		if (Commands::pathExists(destination))
		{
			if (!dependencyCache.contains(destination))
			{
				const std::string commitHash = m_state.tools.getCurrentGitRepositoryHash(destination, m_cleanOutput);
				dependencyCache[destination] = commitHash;
				m_state.cache.setDirty(true);
			}
			if (!inInstallCmd)
				continue;

			update = true;
		}

		const bool submodules = git.submodules();

		const auto& branch = git.branch();
		const auto& tag = git.tag();
		const auto& commit = git.commit();

		const bool tagValid = !tag.empty();
		const bool commitValid = !commit.empty();

		bool res = true;

		if (update)
		{
			const std::string currentBranch = m_state.tools.getCurrentGitRepositoryBranch(destination, m_cleanOutput);
			// LOG('\'', currentBranch, "'  '", branch, '\'');

			// in this case, HEAD means the branch is on a tag or commit
			if ((currentBranch != "HEAD" || commitValid) && currentBranch != branch)
			{
				// we need to fetch a new branch (from a shallow clone), so it's easier to start fresh
				result &= Commands::removeRecursively(destination, m_cleanOutput);
				update = false;
			}

			if (update && commitValid)
			{
				const std::string currentCommit = m_state.tools.getCurrentGitRepositoryHash(destination, m_cleanOutput);
				// LOG('\'', currentTag, "'  '", tag, '\'');

				if (!String::startsWith(commit, currentCommit))
				{
					// we need to fetch a new branch (from a shallow clone), so it's easier to start fresh
					result &= Commands::removeRecursively(destination, m_cleanOutput);
					update = false;
				}
			}

			if (update && tagValid)
			{
				const std::string currentTag = m_state.tools.getCurrentGitRepositoryTag(destination, m_cleanOutput);
				// LOG('\'', currentTag, "'  '", tag, '\'');

				if (currentTag != tag)
				{
					// we need to fetch a new branch (from a shallow clone), so it's easier to start fresh
					result &= Commands::removeRecursively(destination, m_cleanOutput);
					update = false;
				}
			}

			if (update)
			{
				// if commit or tag was specified, we shouldn't update this way
				if (commitValid || tagValid)
					continue;

				if (dependencyCache.contains(destination))
				{
					// We're using a shallow clone, so this only works if the branch hasn't changed
					const std::string originHash = m_state.tools.getCurrentGitRepositoryHashFromRemote(destination, branch, m_cleanOutput);
					const std::string cachedHash = dependencyCache[destination].get<std::string>();

					if (cachedHash == originHash)
						continue;
				}

				const auto& checkoutTo = tagValid ? tag : branch;

				Output::msgUpdatingDependency(repository, checkoutTo);

				res &= m_state.tools.updateGitRepositoryShallow(destination, m_cleanOutput);
				result &= res;
			}
		}

		if (!update)
		{
			const auto& checkoutTo = tagValid ? tag : branch;

			Output::msgFetchingDependency(repository, checkoutTo);

			uint maxJobs = m_state.environment.maxJobs();

			StringList cmd;
			cmd.push_back(m_state.tools.git());
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

			res &= Commands::subprocess(cmd, m_cleanOutput);

			if (commitValid)
			{
				res &= m_state.tools.resetGitRepositoryToCommit(destination, commit, m_cleanOutput);
			}

			result &= res;
		}

		if (res)
		{
			const std::string commitHash = m_state.tools.getCurrentGitRepositoryHash(destination, m_cleanOutput);

			// Output::msgDisplayBlack(commitHash); // useful for debugging
			dependencyCache[destination] = commitHash;
			m_state.cache.setDirty(true);
		}
		else
		{
			result &= Commands::removeRecursively(destination, m_cleanOutput);
		}

		++count;
	}

	if (result)
	{
		StringList eraseList;
		for (auto& [key, value] : dependencyCache.items())
		{
			if (!List::contains(destinationCache, key))
				eraseList.push_back(key);
		}

		for (auto& it : eraseList)
		{
			if (Commands::pathExists(it))
			{
				const bool removed = Commands::removeRecursively(it, m_cleanOutput);
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
			m_state.cache.setDirty(true);
		}

		if (Commands::pathIsEmpty(externalDepDir, {}, true))
			result &= Commands::remove(externalDepDir);
	}

	if (count > 0)
		Output::lineBreak();

	return result;
}
}
