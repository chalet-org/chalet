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

struct GitRunner
{
	explicit GitRunner(StatePrototype& inPrototype, const GitDependency& inDependency);

	bool run(const bool inDoNotUpdate);

private:
	StatePrototype& m_prototype;
	const GitDependency& m_dependency;

	const std::string& m_repository;
	const std::string& m_destination;
	const std::string& m_branch;
	const std::string& m_tag;
	const std::string& m_commit;

	const bool m_submodules;
	bool m_update = false;
};
}

#endif // CHALET_GIT_RUNNER_HPP
