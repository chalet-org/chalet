/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

namespace chalet
{
struct CentralState;
struct GitDependency;
struct ScriptDependency;

class DependencyManager
{
public:
	explicit DependencyManager(CentralState& inCentralState);

	bool run();

private:
	bool runScriptDependency(const ScriptDependency& inTarget);

	StringList getUnusedDependencies() const;
	bool removeUnusedDependencies(const StringList& inList);
	bool removeExternalDependencyDirectoryIfEmpty() const;

	CentralState& m_centralState;

	StringList m_depsChanged;
};
}
