/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

namespace chalet
{
struct CentralState;
struct GitDependency;
class ExternalDependencyCache;

struct GitRunner
{
	explicit GitRunner(CentralState& inCentralState);

	bool run(GitDependency& gitDependency, StringList& outChanged);

private:
	bool gitRepositoryShouldUpdate(const GitDependency& inDependency, const bool inDestinationExists);

	bool fetchDependency(const GitDependency& inDependency, const bool inDestinationExists);
	StringList getCloneCommand(const GitDependency& inDependency);

	bool needsUpdate(const GitDependency& inDependency);
	bool updateDependencyCache(const GitDependency& inDependency);

	void displayCheckingForUpdates(const std::string& inDestination);
	void displayFetchingMessageStart(const GitDependency& inDependency);

	std::string getCurrentGitRepositoryBranch(const std::string& inRepoPath) const;
	std::string getCurrentGitRepositoryTag(const std::string& inRepoPath) const;
	std::string getCurrentGitRepositoryHash(const std::string& inRepoPath) const;
	std::string getCurrentGitRepositoryHashFromOrigin(const std::string& inRepoPath, const std::string& inBranch) const;
	std::string getLatestGitRepositoryHashWithoutClone(const std::string& inRepoPath, const std::string& inBranch) const;
	bool updateGitRepositoryShallow(const std::string& inRepoPath) const;
	bool resetGitRepositoryToCommit(const std::string& inRepoPath, const std::string& inCommit) const;

	std::string getCleanGitPath(const std::string& inPath) const;

	CentralState& m_centralState;
	ExternalDependencyCache& m_dependencyCache;

#if defined(CHALET_WIN32)
	const std::string m_commandPrompt;
#endif
	const std::string m_git;
};
}
