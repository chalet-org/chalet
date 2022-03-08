/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_DEPENDENCY_MANAGER_HPP
#define CHALET_DEPENDENCY_MANAGER_HPP

namespace chalet
{
struct CentralState;
struct GitDependency;

class DependencyManager
{
public:
	explicit DependencyManager(CentralState& inCentralState);

	bool run();

private:
	StringList getUnusedDependencies() const;
	bool removeUnusedDependencies(const StringList& inList);
	bool removeExternalDependencyDirectoryIfEmpty() const;

	CentralState& m_centralState;
};
}

#endif // CHALET_DEPENDENCY_MANAGER_HPP
