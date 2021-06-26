/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_GIT_RUNNER_HPP
#define CHALET_GIT_RUNNER_HPP

namespace chalet
{
struct StatePrototype;
struct GitDependency;
class ExternalDependencyCache;

struct GitRunner
{
	explicit GitRunner(StatePrototype& inPrototype, const GitDependency& inDependency);

	bool run(const bool inDoNotUpdate);

	bool fetched() const noexcept;

private:
	bool gitRepositoryShouldUpdate(const bool inDoNotUpdate);

	StringList getGitCloneCommand(const std::string& inCheckoutTo);

	bool checkBranchForUpdate();
	bool checkCommitForUpdate();
	bool checkTagForUpdate();

	const std::string& getCurrentBranch();
	const std::string& getCurrentCommit();
	const std::string& getCurrentTag();

	StatePrototype& m_prototype;
	const GitDependency& m_dependency;
	ExternalDependencyCache& m_dependencyCache;

	const std::string& m_repository;
	const std::string& m_destination;
	const std::string& m_branch;
	const std::string& m_tag;
	const std::string& m_commit;

	std::string m_currentBranch;
	std::string m_currentCommit;
	std::string m_currentTag;

	const bool m_submodules;
	bool m_update = false;
	bool m_fetched = false;
};
}

#endif // CHALET_GIT_RUNNER_HPP
