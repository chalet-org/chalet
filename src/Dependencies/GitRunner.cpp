/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Dependencies/GitRunner.hpp"

#include <thread>

#include "Cache/ExternalDependencyCache.hpp"
#include "Core/CommandLineInputs.hpp"
#include "State/CentralState.hpp"
#include "State/Dependency/GitDependency.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Output.hpp"
#include "Terminal/Path.hpp"
#include "Utility/String.hpp"
#include "Utility/Timer.hpp"

namespace chalet
{
/*****************************************************************************/
GitRunner::GitRunner(CentralState& inCentralState) :
	m_centralState(inCentralState),
	m_dependencyCache(m_centralState.cache.file().externalDependencies()),
#if defined(CHALET_WIN32)
	m_commandPrompt(m_centralState.tools.commandPrompt()),
#endif
	m_git(m_centralState.tools.git())
{
}

/*****************************************************************************/
bool GitRunner::run(const GitDependency& inDependency, const bool inDoNotUpdate)
{
	if (!gitRepositoryShouldUpdate(inDependency, inDoNotUpdate))
		return true;

	if (fetchDependency(inDependency))
		return updateDependencyCache(inDependency);

	const auto& destination = inDependency.destination();

	if (Commands::pathExists(destination))
		Commands::removeRecursively(destination);

	return false;
}

/*****************************************************************************/
bool GitRunner::fetched() const noexcept
{
	return m_fetched;
}

/*****************************************************************************/
bool GitRunner::gitRepositoryShouldUpdate(const GitDependency& inDependency, const bool inDoNotUpdate)
{
	const auto& destination = inDependency.destination();
	m_destinationExists = Commands::pathExists(destination);
	// During the build, just continue if the path already exists

	if (!m_dependencyCache.contains(destination))
	{
		if (m_destinationExists)
			return Commands::removeRecursively(destination);

		return true;
	}

	const auto& cachedValue = m_dependencyCache.get(destination);
	auto split = String::split(cachedValue, ' ');
	if (split.size() != 5 || split[0].empty())
	{
		if (m_destinationExists)
			return Commands::removeRecursively(destination);

		return true;
	}

	if (!m_destinationExists)
		return true;

	if (inDoNotUpdate)
		return false;

	return needsUpdate(inDependency);
}

/*****************************************************************************/
bool GitRunner::fetchDependency(const GitDependency& inDependency)
{
	if (m_destinationExists && m_dependencyCache.contains(inDependency.destination()))
		return true;

	displayFetchingMessageStart(inDependency);

	StringList cmd = getCloneCommand(inDependency);
	if (!Commands::subprocess(cmd))
		return false;

	const auto& commit = inDependency.commit();
	const auto& destination = inDependency.destination();

	if (!commit.empty())
	{
		if (!resetGitRepositoryToCommit(destination, commit))
			return false;
	}

	m_fetched = true;

	return true;
}

/*****************************************************************************/
StringList GitRunner::getCloneCommand(const GitDependency& inDependency)
{
	StringList ret;

	const auto& destination = inDependency.destination();
	const auto& repository = inDependency.repository();
	const auto& commit = inDependency.commit();
	const bool submodules = inDependency.submodules();

	const auto& checkoutTo = getCheckoutTo(inDependency);

	ret.push_back(m_centralState.tools.git());
	ret.emplace_back("clone");
	ret.emplace_back("--quiet");

	if (!checkoutTo.empty() && !String::equals("HEAD", checkoutTo))
	{
		ret.emplace_back("-b");
		ret.push_back(checkoutTo);
	}

	if (!commit.empty())
	{
		ret.emplace_back("--single-branch");
	}
	else
	{
		ret.emplace_back("--depth");
		ret.emplace_back("1");
	}

	if (submodules)
	{
		uint maxJobs = 0;
		if (m_centralState.inputs().maxJobs().has_value())
			maxJobs = *m_centralState.inputs().maxJobs();
		else
			maxJobs = std::thread::hardware_concurrency();

		ret.emplace_back("--recurse-submodules");
		ret.emplace_back("--shallow-submodules");
		ret.emplace_back("--no-remote-submodules");
		ret.emplace_back("-j");
		ret.emplace_back(std::to_string(maxJobs));
	}

	ret.emplace_back("--config");
	ret.emplace_back("advice.detachedHead=false");
	ret.push_back(repository);
	ret.push_back(destination);

	return ret;
}

/*****************************************************************************/
bool GitRunner::needsUpdate(const GitDependency& inDependency)
{
	const auto& destination = inDependency.destination();
	const auto& repository = inDependency.repository();
	const auto& commit = inDependency.commit();
	const auto& branch = inDependency.branch();
	const auto& tag = inDependency.tag();

	if (!m_dependencyCache.contains(destination))
		return true;

	const auto& cachedValue = m_dependencyCache.get(destination);
	auto split = String::split(cachedValue, ' ');
	if (split.size() != 5)
		return true;

	m_lastCachedCommit = split[0];
	m_lastCachedBranch = split[1];
	const auto& cachedCommit = split[2] == "." ? std::string() : split[2];
	const auto& cachedBranch = split[3] == "." ? std::string() : split[3];
	const auto& cachedTag = split[4] == "." ? std::string() : split[4];

	bool commitNeedsUpdate = !commit.empty() && (!String::startsWith(commit, cachedCommit) || !String::startsWith(commit, m_lastCachedCommit));
	bool branchNeedsUpdate = cachedBranch != branch;
	bool tagNeedsUpdate = cachedTag != tag;

	// LOG(commitNeedsUpdate, branchNeedsUpdate, tagNeedsUpdate, destination);

	bool update = commitNeedsUpdate || branchNeedsUpdate || tagNeedsUpdate;
	if (!update && !m_lastCachedBranch.empty())
	{
		Timer timer;
		displayCheckingForUpdates(destination);

		const auto& refToCheck = !tag.empty() ? tag : m_lastCachedBranch;
		auto latestRemote = getLatestGitRepositoryHashWithoutClone(repository, refToCheck);
		if (commit.empty() && !String::equals(m_lastCachedCommit, latestRemote))
		{
			m_lastCachedCommit = latestRemote;
			update = true;
		}

		Diagnostic::printDone(timer.asString());

		m_fetched = true;
	}

	if (update)
	{
		if (!Commands::removeRecursively(destination))
			return false;

		m_destinationExists = false;
	}

	return update;
}

/*****************************************************************************/
void GitRunner::displayCheckingForUpdates(const std::string& inDestination)
{
	Diagnostic::infoEllipsis("Checking remote for updates: {}", inDestination);
}

/*****************************************************************************/
void GitRunner::displayFetchingMessageStart(const GitDependency& inDependency)
{
	const auto& checkoutTo = getCheckoutTo(inDependency);
	const auto& repository = inDependency.repository();

	Output::msgFetchingDependency(repository, checkoutTo);
}

/*****************************************************************************/
bool GitRunner::updateDependencyCache(const GitDependency& inDependency)
{
	const auto& destination = inDependency.destination();
	const auto& commit = inDependency.commit();
	const auto& branch = inDependency.branch();
	const auto& tag = inDependency.tag();

	m_lastCachedCommit = getCurrentGitRepositoryHash(destination);

	if (m_lastCachedBranch.empty())
	{
		m_lastCachedBranch = getCurrentGitRepositoryBranch(destination);
	}

	auto lastBranch = m_lastCachedBranch.empty() ? "." : m_lastCachedBranch;
	auto valueCommit = commit.empty() ? "." : commit;
	auto valueBranch = branch.empty() ? "." : branch;
	auto valueTag = tag.empty() ? "." : tag;

	auto value = fmt::format("{} {} {} {} {}", m_lastCachedCommit, lastBranch, valueCommit, valueBranch, valueTag);

	if (m_dependencyCache.contains(destination))
	{
		if (m_dependencyCache.get(destination) != value)
		{
			m_dependencyCache.set(destination, std::move(value));
		}
	}
	else
	{
		m_dependencyCache.emplace(destination, std::move(value));
	}

	// Note: Some (bad) repos have source files in the root. using that as an include path
	//   could result in trouble...
	for (auto& path : {
			 ".git",
			 ".gitignore",
			 ".gitattributes",
			 ".gitmodules",
			 ".github",
		 })
	{
		auto outPath = fmt::format("{}/{}", destination, path);
		if (Commands::pathExists(outPath))
		{
#if defined(CHALET_WIN32)
			if (String::equals(".git", path))
			{
				Path::sanitizeForWindows(outPath);
				if (!Commands::subprocess({ m_commandPrompt, "/c", fmt::format("rmdir /q /s {}", outPath) }))
					return false;
			}
			else
#endif
			{
				if (!Commands::removeRecursively(outPath))
					return false;
			}
		}
	}

	return true;
}

/*****************************************************************************/
const std::string& GitRunner::getCheckoutTo(const GitDependency& inDependency)
{
	const auto& branch = inDependency.branch();
	const auto& tag = inDependency.tag();

	return !tag.empty() ? tag : branch;
}

/*****************************************************************************/
std::string GitRunner::getCurrentGitRepositoryBranch(const std::string& inRepoPath) const
{
	std::string branch = Commands::subprocessOutput({ m_git, "-C", inRepoPath, "rev-parse", "--abbrev-ref", "HEAD" });
	return branch;
}

/*****************************************************************************/
std::string GitRunner::getCurrentGitRepositoryTag(const std::string& inRepoPath) const
{
	std::string tag = Commands::subprocessOutput({ m_git, "-C", inRepoPath, "describe", "--tags", "--exact-match", "--abbrev=0" }, PipeOption::Pipe, PipeOption::Close);
	return tag;
}

/*****************************************************************************/
std::string GitRunner::getCurrentGitRepositoryHash(const std::string& inRepoPath) const
{
	std::string hash = Commands::subprocessOutput({ m_git, "-C", inRepoPath, "rev-parse", "--verify", "--quiet", "HEAD" });
	return hash;
}

/*****************************************************************************/
std::string GitRunner::getCurrentGitRepositoryHashFromOrigin(const std::string& inRepoPath, const std::string& inBranch) const
{
	std::string originHash = Commands::subprocessOutput({ m_git, "-C", inRepoPath, "rev-parse", "--verify", "--quiet", fmt::format("origin/{}", inBranch) });
	return originHash;
}

std::string GitRunner::getLatestGitRepositoryHashWithoutClone(const std::string& inRepoPath, const std::string& inBranch) const
{
	std::string result = Commands::subprocessOutput({ m_git, "ls-remote", inRepoPath, inBranch });
	if (result.empty())
		return result;

	auto split = String::split(result, '\n');
	if (split.front().empty())
		return std::string();

	auto ref = String::split(split.front(), '\t');
	return ref.front();
}

/*****************************************************************************/
bool GitRunner::updateGitRepositoryShallow(const std::string& inRepoPath) const
{
	return Commands::subprocess({ m_git, "-C", inRepoPath, "pull", "--quiet", "--update-shallow" });
}

/*****************************************************************************/
bool GitRunner::resetGitRepositoryToCommit(const std::string& inRepoPath, const std::string& inCommit) const
{
	return Commands::subprocess({ m_git, "-C", inRepoPath, "reset", "--quiet", "--hard", inCommit });
}

}
