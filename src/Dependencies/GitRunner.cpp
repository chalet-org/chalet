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

	if (!gitRepositoryShouldUpdate(inDoNotUpdate))
		return true;

	result &= checkBranchForUpdate();
	result &= checkCommitForUpdate();
	result &= checkTagForUpdate();

	auto& dependencyCache = m_prototype.cache.file().externalDependencies();

	const bool tagValid = !m_tag.empty();
	const bool commitValid = !m_commit.empty();

	const auto& branch = !m_branch.empty() ? m_branch : getCurrentBranch();
	const auto& checkoutTo = tagValid ? m_tag : branch;

	if (m_update)
	{
		// commit & tag shouldn't update
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
		auto newCommitHash = m_prototype.tools.getCurrentGitRepositoryHash(m_destination);
		// LOG(newCommitHash);
		dependencyCache.set(m_destination, std::move(newCommitHash));
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

/*****************************************************************************/
bool GitRunner::gitRepositoryShouldUpdate(const bool inDoNotUpdate)
{
	auto& dependencyCache = m_prototype.cache.file().externalDependencies();

	// During the build, just continue if the path already exists
	if (Commands::pathExists(m_destination))
	{
		if (!dependencyCache.contains(m_destination))
		{
			const auto& currentCommit = getCurrentCommit();
			dependencyCache.emplace(m_destination, currentCommit);
		}
		if (inDoNotUpdate)
			return false;

		m_update = true;
	}

	return true;
}

/*****************************************************************************/
bool GitRunner::checkBranchForUpdate()
{
	const bool branchValid = !m_branch.empty();
	if (m_update && branchValid)
	{
		const auto& currentBranch = getCurrentBranch();
		// LOG("branches", fmt::format("'{}'", currentBranch), fmt::format("'{}'", m_branch));

		// in this case, HEAD means the branch is on a tag or commit
		if (currentBranch != "HEAD" && currentBranch != m_branch)
		{
			m_update = false;

			// we need to fetch a new branch (from a shallow clone), so it's easier to start fresh
			if (!Commands::removeRecursively(m_destination))
				return false;
		}
	}

	return true;
}

/*****************************************************************************/
bool GitRunner::checkCommitForUpdate()
{
	const bool commitValid = !m_commit.empty();
	if (m_update && commitValid)
	{
		const auto& currentCommit = getCurrentCommit();
		// LOG("commits:", fmt::format("'{}'", currentCommit), fmt::format("'{}'", m_commit));

		if (!String::startsWith(m_commit, currentCommit))
		{
			m_update = false;

			// we need to fetch a new branch (from a shallow clone), so it's easier to start fresh
			if (!Commands::removeRecursively(m_destination))
				return false;
		}
	}

	return true;
}

/*****************************************************************************/
bool GitRunner::checkTagForUpdate()
{
	const bool tagValid = !m_tag.empty();
	if (m_update && tagValid)
	{
		const auto& currentTag = getCurrentTag();
		// LOG("tags:", fmt::format("'{}'", currentTag), fmt::format("'{}'", m_tag));

		if (currentTag != m_tag)
		{
			m_update = false;

			// we need to fetch a new branch (from a shallow clone), so it's easier to start fresh
			if (!Commands::removeRecursively(m_destination))
				return false;
		}
	}

	return true;
}

/*****************************************************************************/
const std::string& GitRunner::getCurrentBranch()
{
	if (m_currentBranch.empty())
	{
		m_currentBranch = m_prototype.tools.getCurrentGitRepositoryBranch(m_destination);
	}
	return m_currentBranch;
}

/*****************************************************************************/
const std::string& GitRunner::getCurrentCommit()
{
	if (m_currentCommit.empty())
	{
		m_currentCommit = m_prototype.tools.getCurrentGitRepositoryHash(m_destination);
	}
	return m_currentCommit;
}

/*****************************************************************************/
const std::string& GitRunner::getCurrentTag()
{
	if (m_currentTag.empty())
	{
		m_currentTag = m_prototype.tools.getCurrentGitRepositoryTag(m_destination);
	}
	return m_currentTag;
}
}
