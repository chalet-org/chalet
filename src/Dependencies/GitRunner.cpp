/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Dependencies/GitRunner.hpp"

#include <thread>

#include "Cache/ExternalDependencyCache.hpp"
#include "Core/CommandLineInputs.hpp"
#include "Libraries/Json.hpp"
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
bool GitRunner::run(GitDependency& gitDependency)
{
	bool destinationExists = Commands::pathExists(gitDependency.destination());
	if (!gitRepositoryShouldUpdate(gitDependency, destinationExists))
		return true;

	gitDependency.setNeedsUpdate(true);
	destinationExists = Commands::pathExists(gitDependency.destination());
	if (fetchDependency(gitDependency, destinationExists))
	{
		if (!updateDependencyCache(gitDependency))
		{
			Diagnostic::error("Error fetching git dependency: {}", gitDependency.name());
			return false;
		}

		return true;
	}
	else
	{
		const auto& destination = gitDependency.destination();

		if (Commands::pathExists(destination))
			Commands::removeRecursively(destination);
	}

	Diagnostic::error("Error fetching git dependency: {}", gitDependency.name());
	return false;
}

/*****************************************************************************/
bool GitRunner::gitRepositoryShouldUpdate(const GitDependency& inDependency, const bool inDestinationExists)
{
	const auto& destination = inDependency.destination();
	if (!m_dependencyCache.contains(destination))
	{
		if (inDestinationExists)
			return Commands::removeRecursively(destination);

		return true;
	}

	if (!inDestinationExists)
		return true;

	return needsUpdate(inDependency);
}

/*****************************************************************************/
bool GitRunner::fetchDependency(const GitDependency& inDependency, const bool inDestinationExists)
{
	if (inDestinationExists && m_dependencyCache.contains(inDependency.destination()))
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
		u32 maxJobs = 0;
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

	Json json = m_dependencyCache.get(destination);
	if (!json.is_object())
		json = Json::object();

	const auto lastCachedCommit = json["lc"].is_string() ? json["lc"].get<std::string>() : std::string();
	const auto lastCachedBranch = json["lb"].is_string() ? json["lb"].get<std::string>() : std::string();
	const auto cachedCommit = json["c"].is_string() ? json["c"].get<std::string>() : std::string();
	const auto cachedBranch = json["b"].is_string() ? json["b"].get<std::string>() : std::string();
	const auto cachedTag = json["t"].is_string() ? json["t"].get<std::string>() : std::string();

	bool commitNeedsUpdate = !commit.empty() && (!String::startsWith(commit, cachedCommit) || !String::startsWith(commit, lastCachedCommit));
	bool branchNeedsUpdate = cachedBranch != branch;
	bool tagNeedsUpdate = cachedTag != tag;

	// LOG(commitNeedsUpdate, branchNeedsUpdate, tagNeedsUpdate, destination);

	bool update = commitNeedsUpdate || branchNeedsUpdate || tagNeedsUpdate;
	bool isConfigure = m_centralState.inputs().route().isConfigure();
	if (!update && !lastCachedBranch.empty() && isConfigure)
	{
		Timer timer;
		displayCheckingForUpdates(destination);

		const auto& refToCheck = !tag.empty() ? tag : lastCachedBranch;
		auto latestRemote = getLatestGitRepositoryHashWithoutClone(repository, refToCheck);
		if (commit.empty() && !String::equals(lastCachedCommit, latestRemote))
		{
			update = true;
		}

		Diagnostic::printDone(timer.asString());
	}

	if (update)
	{
		if (!Commands::removeRecursively(destination))
			return false;
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

	auto path = getCleanGitPath(repository);
	if (!checkoutTo.empty() && !String::equals("HEAD", checkoutTo))
		path += fmt::format(" ({})", checkoutTo);

	Output::msgFetchingDependency(path);
}

/*****************************************************************************/
bool GitRunner::updateDependencyCache(const GitDependency& inDependency)
{
	const auto& destination = inDependency.destination();
	const auto& commit = inDependency.commit();
	const auto& branch = inDependency.branch();
	const auto& tag = inDependency.tag();

	Json json;
	json["lc"] = getCurrentGitRepositoryHash(destination);
	json["lb"] = getCurrentGitRepositoryBranch(destination);
	json["c"] = commit;
	json["b"] = branch;
	json["t"] = tag;

	if (m_dependencyCache.contains(destination))
	{
		m_dependencyCache.set(destination, std::move(json));
	}
	else
	{
		m_dependencyCache.emplace(destination, std::move(json));
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

/*****************************************************************************/
std::string GitRunner::getCleanGitPath(const std::string& inPath) const
{
	std::string ret = inPath;

	// Common git patterns
	String::replaceAll(ret, "https://", "");
	String::replaceAll(ret, "git@", "");
	String::replaceAll(ret, "git+ssh://", "");
	String::replaceAll(ret, "ssh://", "");
	String::replaceAll(ret, "git://", "");
	// String::replaceAll(ret, "rsync://", "");
	// String::replaceAll(ret, "file://", "");

	// strip the domain
	char searchChar = '/';
	if (String::contains(':', ret))
		searchChar = ':';

	size_t beg = ret.find_first_of(searchChar);
	if (beg != std::string::npos)
	{
		ret = ret.substr(beg + 1);
	}

	// strip .git
	size_t end = ret.find_last_of('.');
	if (end != std::string::npos)
	{
		ret = ret.substr(0, end);
	}

	return ret;
}

}
