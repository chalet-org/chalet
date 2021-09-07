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
struct CommandLineInputs;

struct GitRunner
{
	explicit GitRunner(StatePrototype& inPrototype, const CommandLineInputs& inInputs, const GitDependency& inDependency);

	bool run(const bool inDoNotUpdate);

	bool fetched() const noexcept;

private:
	bool gitRepositoryShouldUpdate(const bool inDoNotUpdate);

	bool fetchDependency();
	StringList getGitCloneCommand(const std::string& inCheckoutTo);

	bool needsUpdate();
	bool updateDependencyCache();

	void displayCheckingForUpdates();
	void displayFetchingMessageStart();

	const std::string& getCheckoutTo();

	StatePrototype& m_prototype;
	const CommandLineInputs& m_inputs;
	const GitDependency& m_dependency;
	ExternalDependencyCache& m_dependencyCache;

	const std::string& m_repository;
	const std::string& m_destination;
	const std::string& m_branch;
	const std::string& m_tag;
	const std::string& m_commit;

	std::string m_lastCachedCommit;
	std::string m_lastCachedBranch;
	std::string m_checkoutBranch;

	const bool m_submodules;
	bool m_destinationExists = false;
	bool m_fetched = false;
};
}

#endif // CHALET_GIT_RUNNER_HPP
