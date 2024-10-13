/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include "Dependencies/IDependencyBuilder.hpp"

namespace chalet
{
struct CentralState;
struct GitDependency;
class ExternalDependencyCache;

struct GitDependencyBuilder final : public IDependencyBuilder
{
	explicit GitDependencyBuilder(CentralState& inCentralState, const GitDependency& inDependency);

	virtual bool validateRequiredTools() const final;
	virtual bool resolveDependency(StringList& outChanged) final;

private:
	bool localPathShouldUpdate(const bool inDestinationExists);

	bool fetchDependency(const bool inDestinationExists);
	StringList getCloneCommand();

	bool needsUpdate();
	bool updateDependencyCache();

	void displayFetchingMessageStart();

	std::string getCurrentGitRepositoryBranch(const std::string& inRepoPath) const;
	std::string getCurrentGitRepositoryTag(const std::string& inRepoPath) const;
	std::string getCurrentGitRepositoryHash(const std::string& inRepoPath) const;
	std::string getCurrentGitRepositoryHashFromOrigin(const std::string& inRepoPath, const std::string& inBranch) const;
	std::string getLatestGitRepositoryHashWithoutClone(const std::string& inRepoPath, const std::string& inBranch) const;
	bool updateGitRepositoryShallow(const std::string& inRepoPath) const;
	bool resetGitRepositoryToCommit(const std::string& inRepoPath, const std::string& inCommit) const;

	std::string getCleanGitPath(const std::string& inPath) const;

	const GitDependency& m_gitDependency;

#if defined(CHALET_WIN32)
	const std::string m_commandPrompt;
#endif
	const std::string m_git;
};
}
