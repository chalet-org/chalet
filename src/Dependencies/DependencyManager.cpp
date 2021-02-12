/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Dependencies/DependencyManager.hpp"

#include "Libraries/Format.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Output.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"
#include "Json/JsonComments.hpp"

// TODO: Buckaroo integration?
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

	const auto& externalDepDir = m_state.environment.externalDepDir();
	auto& environmentCache = m_state.cache.environmentCache();

	Json& dependencyCache = environmentCache.json["externalDependencies"];

	StringList destinationCache;

	bool result = true;
	int count = 0;
	for (auto& dep : m_state.externalDependencies)
	{
		const auto& repository = dep->repository();
		const auto& destination = dep->destination();
		destinationCache.push_back(destination);

		if (destination.empty() || String::startsWith(".", destination) || String::startsWith("/", destination))
		{
			// This shouldn't occur, but would be pretty bad if it did
			Diagnostic::errorAbort(fmt::format("The external dependency destination was blank for '{}'.", repository));
			return false;
		}

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

		const bool submodules = dep->submodules();

		const auto& branch = dep->branch();
		const auto& tag = dep->tag();
		const auto& commit = dep->commit();

		const bool tagValid = !tag.empty();
		const bool commitValid = !commit.empty();

		std::string cmd;
		bool res = true;

		if (update)
		{
			const std::string currentBranch = m_state.tools.getCurrentGitRepositoryBranch(destination, m_cleanOutput);
			// LOG("'", currentBranch, "'  '", branch, "'");

			// in this case, HEAD means the branch is on a tag or commit
			if ((currentBranch != "HEAD" || commitValid) && currentBranch != branch)
			{
				// we need to fetch a new branch (from a shallow clone), so it's easier to start fresh
				result &= Commands::shellRemove(destination, m_cleanOutput);
				update = false;
			}

			if (update && commitValid)
			{
				const std::string currentCommit = m_state.tools.getCurrentGitRepositoryHash(destination, m_cleanOutput);
				// LOG("'", currentTag, "'  '", tag, "'");

				if (!String::startsWith(commit, currentCommit))
				{
					// we need to fetch a new branch (from a shallow clone), so it's easier to start fresh
					result &= Commands::shellRemove(destination, m_cleanOutput);
					update = false;
				}
			}

			if (update && tagValid)
			{
				const std::string currentTag = m_state.tools.getCurrentGitRepositoryTag(destination, m_cleanOutput);
				// LOG("'", currentTag, "'  '", tag, "'");

				if (currentTag != tag)
				{
					// we need to fetch a new branch (from a shallow clone), so it's easier to start fresh
					result &= Commands::shellRemove(destination, m_cleanOutput);
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
			const std::string submoduleArg = !submodules ? "" : fmt::format(" --recurse-submodules --shallow-submodules --no-remote-submodules -j {}", maxJobs);

			const std::string method = commitValid ? "--single-branch" : "--depth 1";
			cmd = fmt::format("git clone --quiet -b {checkoutTo} {method}{submoduleArg} --config advice.detachedHead=false {repository} {destination}",
				FMT_ARG(checkoutTo),
				FMT_ARG(method),
				FMT_ARG(submoduleArg),
				FMT_ARG(repository),
				FMT_ARG(destination));

			// LOG(cmd);

			res &= Commands::shell(cmd, m_cleanOutput);

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
			result &= Commands::shellRemove(destination, m_cleanOutput);
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
				const bool removed = Commands::shellRemove(it, m_cleanOutput);
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

		if (Commands::pathIsEmpty(externalDepDir, true))
			result &= Commands::remove(externalDepDir);
	}

	if (count > 0)
		Output::lineBreak();

	return result;
}
}
