/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Dependencies/GitDependencyBuilder.hpp"

#include <thread>

#include "Cache/ExternalDependencyCache.hpp"
#include "Core/CommandLineInputs.hpp"
#include "Libraries/Json.hpp"
#include "Process/Process.hpp"
#include "State/CentralState.hpp"
#include "State/Dependency/GitDependency.hpp"
#include "System/Files.hpp"
#include "Terminal/Output.hpp"
#include "Utility/Path.hpp"
#include "Utility/String.hpp"
#include "Utility/Timer.hpp"

namespace chalet
{
/*****************************************************************************/
GitDependencyBuilder::GitDependencyBuilder(CentralState& inCentralState, const GitDependency& inDependency) :
	IDependencyBuilder(inCentralState),
	m_gitDependency(inDependency),
#if defined(CHALET_WIN32)
	m_commandPrompt(m_centralState.tools.commandPrompt()),
#endif
	m_git(m_centralState.tools.git())
{
}

/*****************************************************************************/
bool GitDependencyBuilder::validateRequiredTools() const
{
	if (m_git.empty())
	{
		Diagnostic::error("Git dependency '{}' requested, but git is not installed", m_gitDependency.name());
		return false;
	}

	return true;
}

/*****************************************************************************/
bool GitDependencyBuilder::resolveDependency(StringList& outChanged)
{
	const auto& destination = m_gitDependency.destination();

	bool destinationExists = Files::pathExists(destination);
	if (!localPathShouldUpdate(destinationExists))
		return true;

	destinationExists = Files::pathExists(destination);
	if (fetchDependency(destinationExists))
	{
		if (!updateDependencyCache())
		{
			Diagnostic::error("Error fetching git dependency: {}", m_gitDependency.name());
			return false;
		}

		outChanged.emplace_back(Files::getCanonicalPath(destination));
		return true;
	}
	else
	{
		if (Files::pathExists(destination))
			Files::removeRecursively(destination);
	}

	Diagnostic::error("Error fetching git dependency: {}", m_gitDependency.name());
	return false;
}

/*****************************************************************************/
bool GitDependencyBuilder::localPathShouldUpdate(const bool inDestinationExists)
{
	const auto& destination = m_gitDependency.destination();
	if (!m_dependencyCache.contains(destination))
	{
		if (inDestinationExists)
			return Files::removeRecursively(destination);

		return true;
	}

	if (!inDestinationExists)
		return true;

	return needsUpdate();
}

/*****************************************************************************/
bool GitDependencyBuilder::fetchDependency(const bool inDestinationExists)
{
	const auto& destination = m_gitDependency.destination();

	if (inDestinationExists && m_dependencyCache.contains(destination))
		return true;

	displayFetchingMessageStart();

	StringList cmd = getCloneCommand();
	if (!Process::run(cmd))
		return false;

	const auto& commit = m_gitDependency.commit();

	if (!commit.empty())
	{
		if (!resetGitRepositoryToCommit(destination, commit))
			return false;
	}

	return true;
}

/*****************************************************************************/
StringList GitDependencyBuilder::getCloneCommand()
{
	StringList ret;

	const auto& destination = m_gitDependency.destination();
	const auto& repository = m_gitDependency.repository();
	const auto& branch = m_gitDependency.branch();
	const auto& tag = m_gitDependency.tag();
	const auto& commit = m_gitDependency.commit();
	const bool submodules = m_gitDependency.submodules();

	const auto& checkoutTo = !tag.empty() ? tag : branch;

	ret.push_back(m_git);
	ret.emplace_back("clone");
	ret.emplace_back("--quiet");

	if (!checkoutTo.empty() && !String::equals("HEAD", checkoutTo))
	{
		ret.emplace_back("--branch");
		ret.push_back(checkoutTo);
	}

	if (checkoutTo.empty() && !commit.empty())
	{
		ret.emplace_back("--single-branch");
	}
	else if (commit.empty())
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
bool GitDependencyBuilder::needsUpdate()
{
	const auto& destination = m_gitDependency.destination();
	const auto& repository = m_gitDependency.repository();
	const auto& commit = m_gitDependency.commit();
	const auto& branch = m_gitDependency.branch();
	const auto& tag = m_gitDependency.tag();

	if (!m_dependencyCache.contains(destination))
		return true;

	Json jRoot = m_dependencyCache.get(destination);
	if (!jRoot.is_object())
		jRoot = Json::object();

	const auto lastCachedCommit = json::get<std::string>(jRoot, "lc");
	const auto lastCachedBranch = json::get<std::string>(jRoot, "lb");
	const auto cachedCommit = json::get<std::string>(jRoot, "c");
	const auto cachedBranch = json::get<std::string>(jRoot, "b");
	const auto cachedTag = json::get<std::string>(jRoot, "t");

	const bool commitNeedsUpdate = (!commit.empty() && (!String::startsWith(commit, cachedCommit) || !String::startsWith(commit, lastCachedCommit))) || (commit.empty() && !cachedCommit.empty());
	const bool branchNeedsUpdate = cachedBranch != branch;
	const bool tagNeedsUpdate = cachedTag != tag;

	// LOG(commitNeedsUpdate, branchNeedsUpdate, tagNeedsUpdate, destination);

	const bool isConfigure = m_centralState.inputs().route().isConfigure();
	bool update = commitNeedsUpdate || branchNeedsUpdate || tagNeedsUpdate;
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
		if (!Files::removeRecursively(destination))
			return false;
	}

	return update;
}

/*****************************************************************************/
void GitDependencyBuilder::displayFetchingMessageStart()
{
	const auto& branch = m_gitDependency.branch();
	const auto& tag = m_gitDependency.tag();
	const auto& checkoutTo = !tag.empty() ? tag : branch;

	const auto& repository = m_gitDependency.repository();

	auto path = getCleanGitPath(repository);
	if (!checkoutTo.empty() && !String::equals("HEAD", checkoutTo))
		path += fmt::format(" ({})", checkoutTo);

	Output::msgFetchingDependency(path);
}

/*****************************************************************************/
bool GitDependencyBuilder::updateDependencyCache()
{
	const auto& destination = m_gitDependency.destination();
	const auto& commit = m_gitDependency.commit();
	const auto& branch = m_gitDependency.branch();
	const auto& tag = m_gitDependency.tag();

	Json data;
	data["lc"] = getCurrentGitRepositoryHash(destination);
	data["lb"] = getCurrentGitRepositoryBranch(destination);
	data["c"] = commit;
	data["b"] = branch;
	data["t"] = tag;

	if (m_dependencyCache.contains(destination))
	{
		m_dependencyCache.set(destination, std::move(data));
	}
	else
	{
		m_dependencyCache.emplace(destination, std::move(data));
	}

	// Note: Some repos have source files in the root. using that as an include path
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
		if (Files::pathExists(outPath))
		{
#if defined(CHALET_WIN32)
			if (String::equals(".git", path))
			{
				Path::toWindows(outPath);
				if (!Process::run({ m_commandPrompt, "/c", fmt::format("rmdir /q /s {}", outPath) }))
					return false;
			}
			else
#endif
			{
				if (!Files::removeRecursively(outPath))
					return false;
			}
		}
	}

	return true;
}

/*****************************************************************************/
std::string GitDependencyBuilder::getCurrentGitRepositoryBranch(const std::string& inRepoPath) const
{
	std::string branch = Process::runOutput({ m_git, "-C", inRepoPath, "rev-parse", "--abbrev-ref", "HEAD" });
	return branch;
}

/*****************************************************************************/
std::string GitDependencyBuilder::getCurrentGitRepositoryTag(const std::string& inRepoPath) const
{
	std::string tag = Process::runOutput({ m_git, "-C", inRepoPath, "describe", "--tags", "--exact-match", "--abbrev=0" }, PipeOption::Pipe, PipeOption::Close);
	return tag;
}

/*****************************************************************************/
std::string GitDependencyBuilder::getCurrentGitRepositoryHash(const std::string& inRepoPath) const
{
	std::string hash = Process::runOutput({ m_git, "-C", inRepoPath, "rev-parse", "--verify", "--quiet", "HEAD" });
	return hash;
}

/*****************************************************************************/
std::string GitDependencyBuilder::getCurrentGitRepositoryHashFromOrigin(const std::string& inRepoPath, const std::string& inBranch) const
{
	std::string originHash = Process::runOutput({ m_git, "-C", inRepoPath, "rev-parse", "--verify", "--quiet", fmt::format("origin/{}", inBranch) });
	return originHash;
}

std::string GitDependencyBuilder::getLatestGitRepositoryHashWithoutClone(const std::string& inRepoPath, const std::string& inBranch) const
{
	std::string result = Process::runOutput({ m_git, "ls-remote", inRepoPath, inBranch });
	if (result.empty())
		return result;

	auto split = String::split(result, '\n');
	if (split.front().empty())
		return std::string();

	auto ref = String::split(split.front(), '\t');
	return ref.front();
}

/*****************************************************************************/
bool GitDependencyBuilder::updateGitRepositoryShallow(const std::string& inRepoPath) const
{
	return Process::run({ m_git, "-C", inRepoPath, "pull", "--quiet", "--update-shallow" });
}

/*****************************************************************************/
bool GitDependencyBuilder::resetGitRepositoryToCommit(const std::string& inRepoPath, const std::string& inCommit) const
{
	return Process::run({ m_git, "-C", inRepoPath, "reset", "--quiet", "--hard", inCommit });
}

/*****************************************************************************/
std::string GitDependencyBuilder::getCleanGitPath(const std::string& inPath) const
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
