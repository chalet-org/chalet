/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_DEPENDENCY_MANAGER_HPP
#define CHALET_DEPENDENCY_MANAGER_HPP

namespace chalet
{
struct StatePrototype;
struct GitDependency;
struct CommandLineInputs;

class DependencyManager
{
public:
	explicit DependencyManager(const CommandLineInputs& inInputs, StatePrototype& inPrototype);

	bool run();

private:
	bool runGitDependency(const GitDependency& inDependency);

	StringList getUnusedDependencies() const;
	bool removeUnusedDependencies(const StringList& inList) const;
	bool removeExternalDependencyDirectoryIfEmpty() const;

	const CommandLineInputs& m_inputs;
	StatePrototype& m_prototype;

	StringList m_destinationCache;
};
}

#endif // CHALET_DEPENDENCY_MANAGER_HPP
