/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Dependencies/IDependencyBuilder.hpp"

#include "Cache/ExternalDependencyCache.hpp"
#include "Cache/WorkspaceInternalCacheFile.hpp"
#include "State/CentralState.hpp"
#include "State/Dependency/ArchiveDependency.hpp"
#include "State/Dependency/GitDependency.hpp"

#include "Dependencies/ArchiveDependencyBuilder.hpp"
#include "Dependencies/GitDependencyBuilder.hpp"

namespace chalet
{
/*****************************************************************************/
[[nodiscard]] Unique<IDependencyBuilder> IDependencyBuilder::make(CentralState& inCentralState, IExternalDependency& inDependency)
{
	switch (inDependency.type())
	{
		case ExternalDependencyType::Archive:
			return std::make_unique<ArchiveDependencyBuilder>(inCentralState, static_cast<ArchiveDependency&>(inDependency));
		case ExternalDependencyType::Git:
			return std::make_unique<GitDependencyBuilder>(inCentralState, static_cast<GitDependency&>(inDependency));
		default: break;
	}

	return nullptr;
}

/*****************************************************************************/
IDependencyBuilder::IDependencyBuilder(CentralState& inCentralState) :
	m_centralState(inCentralState),
	m_dependencyCache(m_centralState.cache.file().externalDependencies())
{
	UNUSED(m_centralState);
	UNUSED(m_dependencyCache);
}

/*****************************************************************************/
void IDependencyBuilder::displayCheckingForUpdates(const std::string& inDestination)
{
	Diagnostic::infoEllipsis("Checking remote for updates: {}", inDestination);
}
}
