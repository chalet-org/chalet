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

struct GitDependencyBuilder
{
	explicit GitDependencyBuilder(CentralState& inCentralState, const GitDependency& inDependency);

	bool run(StringList& outChanged);

private:
	bool localPathShouldUpdate(const bool inDestinationExists);

	bool fetchDependency(const bool inDestinationExists);
	StringList getCloneCommand();

	bool needsUpdate();
	bool updateDependencyCache();

	void displayCheckingForUpdates(const std::string& inDestination);
	void displayFetchingMessageStart();

	std::string getCurrentGitRepositoryBranch(const std::string& inRepoPath) const;
	std::string getCurrentGitRepositoryTag(const std::string& inRepoPath) const;
	std::string getCurrentGitRepositoryHash(const std::string& inRepoPath) const;
	std::string getCurrentGitRepositoryHashFromOrigin(const std::string& inRepoPath, const std::string& inBranch) const;
	std::string getLatestGitRepositoryHashWithoutClone(const std::string& inRepoPath, const std::string& inBranch) const;
	bool updateGitRepositoryShallow(const std::string& inRepoPath) const;
	bool resetGitRepositoryToCommit(const std::string& inRepoPath, const std::string& inCommit) const;

	std::string getCleanGitPath(const std::string& inPath) const;

	CentralState& m_centralState;
	const GitDependency& m_gitDependency;
	ExternalDependencyCache& m_dependencyCache;

#if defined(CHALET_WIN32)
	const std::string m_commandPrompt;
#endif
	const std::string m_git;
};
}
