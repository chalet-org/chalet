/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Dependencies/GitRunner.hpp"

#include "State/Dependency/GitDependency.hpp"
#include "State/StatePrototype.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Output.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
GitRunner::GitRunner(StatePrototype& inPrototype, const GitDependency& inDependency) :
	m_prototype(inPrototype),
	m_dependency(inDependency),
	m_repository(m_dependency.repository()),
	m_destination(m_dependency.destination()),
	m_branch(m_dependency.branch()),
	m_tag(m_dependency.tag()),
	m_commit(m_dependency.commit()),
	m_submodules(m_dependency.submodules())
{
}

/*****************************************************************************/
bool GitRunner::run(const bool inDoNotUpdate)
{
	bool result = true;
	m_update = false;

	auto& dependencyCache = m_prototype.cache.file().externalDependencies();

	std::string currentBranch;

	// During the build, just continue if the path already exists
	//   (the install command can be used for more complicated tasks)
	// LOG(m_destination);
	if (Commands::pathExists(m_destination))
	{
		if (!dependencyCache.contains(m_destination))
		{
			auto commitHash = m_prototype.tools.getCurrentGitRepositoryHash(m_destination);
			dependencyCache.emplace(m_destination, std::move(commitHash));
		}
		if (inDoNotUpdate)
			return true;

		currentBranch = m_prototype.tools.getCurrentGitRepositoryBranch(m_destination);
		m_update = true;
	}

	const bool branchValid = !m_branch.empty();
	const bool tagValid = !m_tag.empty();
	const bool commitValid = !m_commit.empty();

	if (m_update && branchValid)
	{
		// LOG("branches", fmt::format("'{}'", currentBranch), fmt::format("'{}'", m_branch));

		// in this case, HEAD means the branch is on a tag or commit
		if (currentBranch != "HEAD" && currentBranch != m_branch)
		{
			// we need to fetch a new branch (from a shallow clone), so it's easier to start fresh
			result &= Commands::removeRecursively(m_destination);
			m_update = false;
		}
	}

	if (m_update && commitValid)
	{
		auto currentCommit = m_prototype.tools.getCurrentGitRepositoryHash(m_destination);
		// LOG("commits:", fmt::format("'{}'", currentCommit), fmt::format("'{}'", m_commit));

		if (!String::startsWith(m_commit, currentCommit))
		{
			// we need to fetch a new branch (from a shallow clone), so it's easier to start fresh
			result &= Commands::removeRecursively(m_destination);
			m_update = false;
		}
	}

	if (m_update && tagValid)
	{
		const std::string currentTag = m_prototype.tools.getCurrentGitRepositoryTag(m_destination);
		// LOG("tags:", fmt::format("'{}'", currentTag), fmt::format("'{}'", m_tag));

		if (currentTag != m_tag)
		{
			// we need to fetch a new branch (from a shallow clone), so it's easier to start fresh
			result &= Commands::removeRecursively(m_destination);
			m_update = false;
		}
	}

	const auto& branch = !m_branch.empty() ? m_branch : currentBranch;
	const auto& checkoutTo = tagValid ? m_tag : branch;

	if (m_update)
	{
		// if commit or tag was specified, we shouldn't update this way
		if (commitValid || tagValid)
			return true;

		if (dependencyCache.contains(m_destination))
		{
			// We're using a shallow clone, so this only works if the branch hasn't changed
			const std::string originHash = m_prototype.tools.getCurrentGitRepositoryHashFromOrigin(m_destination, branch);
			const auto& cachedHash = dependencyCache.get(m_destination);

			if (cachedHash == originHash)
				return true;
		}

		Output::msgUpdatingDependency(m_repository, checkoutTo);

		result &= m_prototype.tools.updateGitRepositoryShallow(m_destination);
		m_fetched = true;
	}
	else
	{
		Output::msgFetchingDependency(m_repository, checkoutTo);

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
		if (m_submodules)
		{
			cmd.push_back("--recurse-submodules");
			cmd.push_back("--shallow-submodules");
			cmd.push_back("--no-remote-submodules");
			cmd.push_back("-j");
			cmd.push_back(std::to_string(maxJobs));
		}
		cmd.push_back("--config");
		cmd.push_back("advice.detachedHead=false");
		cmd.push_back(m_repository);
		cmd.push_back(m_destination);

		result &= Commands::subprocess(cmd);

		if (commitValid)
		{
			result &= m_prototype.tools.resetGitRepositoryToCommit(m_destination, m_commit);
		}
		m_fetched = true;
	}

	if (result)
	{
		auto commitHash = m_prototype.tools.getCurrentGitRepositoryHash(m_destination);

		// Output::msgDisplayBlack(commitHash); // useful for debugging
		dependencyCache.set(m_destination, std::move(commitHash));
	}
	else
	{
		result &= Commands::removeRecursively(m_destination);
	}

	return result;
}

/*****************************************************************************/
bool GitRunner::fetched() const noexcept
{
	return m_fetched;
}
}
