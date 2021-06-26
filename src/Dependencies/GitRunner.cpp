/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Dependencies/GitRunner.hpp"

#include "Cache/ExternalDependencyCache.hpp"
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
	m_dependencyCache(m_prototype.cache.file().externalDependencies()),
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

	const bool tagValid = !m_tag.empty();
	const bool commitValid = !m_commit.empty();

	if (m_update)
	{
		const auto& checkoutTo = getCheckoutTo();
		// commit & tag shouldn't update
		if (commitValid || tagValid)
			return true;

		if (m_dependencyCache.contains(m_destination))
		{
			// We're using a shallow clone, so this only works if the branch hasn't changed
			const std::string originHash = m_prototype.tools.getCurrentGitRepositoryHashFromOrigin(m_destination, m_checkoutBranch);
			const auto& cachedHash = m_dependencyCache.get(m_destination);

			if (cachedHash == originHash)
				return true;
		}

		Output::msgUpdatingDependency(m_repository, checkoutTo);

		result &= m_prototype.tools.updateGitRepositoryShallow(m_destination);
		m_fetched = true;
	}
	else
	{
		result &= fetchDependency();
	}

	if (result)
	{
		auto newCommitHash = m_prototype.tools.getCurrentGitRepositoryHash(m_destination);
		// LOG(newCommitHash);
		m_dependencyCache.set(m_destination, std::move(newCommitHash));
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
	m_destinationExists = Commands::pathExists(m_destination);
	// During the build, just continue if the path already exists
	if (m_destinationExists)
	{
		if (!m_dependencyCache.contains(m_destination))
		{
			const auto& currentCommit = getCurrentCommit();
			m_dependencyCache.emplace(m_destination, currentCommit);
		}
		if (inDoNotUpdate)
			return false;

		m_update = true;
	}

	return true;
}

/*****************************************************************************/
bool GitRunner::fetchDependency()
{
	const auto& checkoutTo = getCheckoutTo();

	Output::msgFetchingDependency(m_repository, checkoutTo);

	StringList cmd = getGitCloneCommand(checkoutTo);
	if (!Commands::subprocess(cmd))
		return false;

	if (!m_commit.empty())
	{
		if (!m_prototype.tools.resetGitRepositoryToCommit(m_destination, m_commit))
			return false;
	}
	m_fetched = true;

	return true;
}

/*****************************************************************************/
StringList GitRunner::getGitCloneCommand(const std::string& inCheckoutTo)
{
	StringList ret;

	ret.push_back(m_prototype.tools.git());
	ret.push_back("clone");
	ret.push_back("--quiet");

	if (!inCheckoutTo.empty() && !String::equals("HEAD", inCheckoutTo))
	{
		ret.push_back("-b");
		ret.push_back(inCheckoutTo);
	}

	if (!m_commit.empty())
	{
		ret.push_back("--single-branch");
	}
	else
	{
		ret.push_back("--depth");
		ret.push_back("1");
	}

	if (m_submodules)
	{
		uint maxJobs = m_prototype.environment.maxJobs();

		ret.push_back("--recurse-submodules");
		ret.push_back("--shallow-submodules");
		ret.push_back("--no-remote-submodules");
		ret.push_back("-j");
		ret.push_back(std::to_string(maxJobs));
	}

	ret.push_back("--config");
	ret.push_back("advice.detachedHead=false");
	ret.push_back(m_repository);
	ret.push_back(m_destination);

	return ret;
}

/*****************************************************************************/
bool GitRunner::checkBranchForUpdate()
{
	const bool branchValid = !m_branch.empty();
	if (m_update && m_destinationExists)
	{
		const auto& currentBranch = getCurrentBranch();
		LOG("branches", fmt::format("'{}'", currentBranch), fmt::format("'{}'", m_branch));

		// in this case, HEAD means the branch is on a tag or commit
		if ((currentBranch != m_branch && branchValid) || (m_tag.empty() && m_commit.empty() && String::equals("HEAD", currentBranch)))
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
		LOG("commits:", fmt::format("'{}'", currentCommit), fmt::format("'{}'", m_commit));

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
		LOG("tags:", fmt::format("'{}'", currentTag), fmt::format("'{}'", m_tag));

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
		LOG(m_currentBranch);
	}
	return m_currentBranch;
}

/*****************************************************************************/
const std::string& GitRunner::getCurrentCommit()
{
	if (m_currentCommit.empty())
	{
		m_currentCommit = m_prototype.tools.getCurrentGitRepositoryHash(m_destination);
		LOG(m_currentCommit);
	}
	return m_currentCommit;
}

/*****************************************************************************/
const std::string& GitRunner::getCurrentTag()
{
	if (m_currentTag.empty())
	{
		m_currentTag = m_prototype.tools.getCurrentGitRepositoryTag(m_destination);
		LOG(m_currentTag);
	}
	return m_currentTag;
}

/*****************************************************************************/
const std::string& GitRunner::getCheckoutTo()
{
	if (m_checkoutBranch.empty())
	{
		m_checkoutBranch = m_branch;
		if (m_checkoutBranch.empty())
		{
			if (m_destinationExists)
				m_checkoutBranch = getCurrentBranch();
		}
	}

	return !m_tag.empty() ? m_tag : m_checkoutBranch;
}
}
