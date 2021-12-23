/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Dependencies/GitRunner.hpp"

#include <thread>

#include "Cache/ExternalDependencyCache.hpp"
#include "Core/CommandLineInputs.hpp"
#include "State/Dependency/GitDependency.hpp"
#include "State/StatePrototype.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Output.hpp"
#include "Terminal/Path.hpp"
#include "Utility/String.hpp"
#include "Utility/Timer.hpp"

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
	if (!gitRepositoryShouldUpdate(inDoNotUpdate))
		return true;

	if (fetchDependency())
	{
		return updateDependencyCache();
	}

	if (Commands::pathExists(m_destination))
		Commands::removeRecursively(m_destination);

	return false;
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

	if (!m_dependencyCache.contains(m_destination))
	{
		if (m_destinationExists)
			return Commands::removeRecursively(m_destination);

		return true;
	}

	const auto& cachedValue = m_dependencyCache.get(m_destination);
	auto split = String::split(cachedValue, ' ');
	if (split.size() != 5 || split[0].empty())
	{
		if (m_destinationExists)
			return Commands::removeRecursively(m_destination);

		return true;
	}

	if (!m_destinationExists)
		return true;

	if (inDoNotUpdate)
		return false;

	return needsUpdate();
}

/*****************************************************************************/
bool GitRunner::fetchDependency()
{
	if (m_destinationExists && m_dependencyCache.contains(m_destination))
		return true;

	const auto& checkoutTo = getCheckoutTo();
	displayFetchingMessageStart();

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
	ret.emplace_back("clone");
	ret.emplace_back("--quiet");

	if (!inCheckoutTo.empty() && !String::equals("HEAD", inCheckoutTo))
	{
		ret.emplace_back("-b");
		ret.push_back(inCheckoutTo);
	}

	if (!m_commit.empty())
	{
		ret.emplace_back("--single-branch");
	}
	else
	{
		ret.emplace_back("--depth");
		ret.emplace_back("1");
	}

	if (m_submodules)
	{
		uint maxJobs = 0;
		if (m_prototype.inputs().maxJobs().has_value())
			maxJobs = *m_prototype.inputs().maxJobs();
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
	ret.push_back(m_repository);
	ret.push_back(m_destination);

	return ret;
}

/*****************************************************************************/
bool GitRunner::needsUpdate()
{
	if (!m_dependencyCache.contains(m_destination))
		return true;

	const auto& cachedValue = m_dependencyCache.get(m_destination);
	auto split = String::split(cachedValue, ' ');
	if (split.size() != 5)
		return true;

	m_lastCachedCommit = split[0];
	m_lastCachedBranch = split[1];
	const auto& cachedCommit = split[2] == "." ? std::string() : split[2];
	const auto& cachedBranch = split[3] == "." ? std::string() : split[3];
	const auto& cachedTag = split[4] == "." ? std::string() : split[4];

	bool commitNeedsUpdate = !m_commit.empty() && (!String::startsWith(m_commit, cachedCommit) || !String::startsWith(m_commit, m_lastCachedCommit));
	bool branchNeedsUpdate = cachedBranch != m_branch;
	bool tagNeedsUpdate = cachedTag != m_tag;

	// LOG(commitNeedsUpdate, branchNeedsUpdate, tagNeedsUpdate, m_destination);

	bool update = commitNeedsUpdate || branchNeedsUpdate || tagNeedsUpdate;
	if (!update && !m_lastCachedBranch.empty())
	{
		Timer timer;
		displayCheckingForUpdates();

		const auto& refToCheck = !m_tag.empty() ? m_tag : m_lastCachedBranch;
		auto latestRemote = m_prototype.tools.getLatestGitRepositoryHashWithoutClone(m_repository, refToCheck);
		if (latestRemote != m_lastCachedCommit)
		{
			m_lastCachedCommit = latestRemote;
			update = true;
		}

		Diagnostic::printDone(timer.asString());

		m_fetched = true;
	}

	if (update)
	{
		if (!Commands::removeRecursively(m_destination))
			return false;

		m_destinationExists = false;
	}

	return update;
}

/*****************************************************************************/
void GitRunner::displayCheckingForUpdates()
{
	Diagnostic::infoEllipsis("Checking remote for updates: {}", m_destination);
}

/*****************************************************************************/
void GitRunner::displayFetchingMessageStart()
{
	const auto& checkoutTo = getCheckoutTo();
	Output::msgFetchingDependency(m_repository, checkoutTo);
}

/*****************************************************************************/
bool GitRunner::updateDependencyCache()
{
	m_lastCachedCommit = m_prototype.tools.getCurrentGitRepositoryHash(m_destination);

	if (m_lastCachedBranch.empty())
	{
		m_lastCachedBranch = m_prototype.tools.getCurrentGitRepositoryBranch(m_destination);
	}

	auto lastBranch = m_lastCachedBranch.empty() ? "." : m_lastCachedBranch;
	auto commit = m_commit.empty() ? "." : m_commit;
	auto branch = m_branch.empty() ? "." : m_branch;
	auto tag = m_tag.empty() ? "." : m_tag;

	auto value = fmt::format("{} {} {} {} {}", m_lastCachedCommit, lastBranch, commit, branch, tag);

	if (m_dependencyCache.contains(m_destination))
	{
		if (m_dependencyCache.get(m_destination) != value)
		{
			m_dependencyCache.set(m_destination, std::move(value));
		}
	}
	else
	{
		m_dependencyCache.emplace(m_destination, std::move(value));
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
		auto outPath = fmt::format("{}/{}", m_destination, path);
		if (Commands::pathExists(outPath))
		{
#if defined(CHALET_WIN32)
			if (String::equals(".git", path))
			{
				Path::sanitizeForWindows(outPath);
				if (!Commands::subprocess({ m_prototype.tools.commandPrompt(), "/c", fmt::format("rmdir /q /s {}", outPath) }))
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
const std::string& GitRunner::getCheckoutTo()
{
	if (m_checkoutBranch.empty())
	{
		m_checkoutBranch = m_branch;
	}

	return !m_tag.empty() ? m_tag : m_checkoutBranch;
}
}
