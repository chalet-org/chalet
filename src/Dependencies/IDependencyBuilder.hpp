/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include "State/Dependency/ExternalDependencyType.hpp"
#include "State/Dependency/IExternalDependency.hpp"

namespace chalet
{
struct CentralState;
class ExternalDependencyCache;

struct IDependencyBuilder
{
	[[nodiscard]] static Unique<IDependencyBuilder> make(CentralState& inCentralState, IExternalDependency& inDependency);

	IDependencyBuilder(CentralState& inCentralState);
	CHALET_DISALLOW_COPY_MOVE(IDependencyBuilder);
	virtual ~IDependencyBuilder() = default;

	virtual bool validateRequiredTools() const = 0;
	virtual bool resolveDependency(StringList& outChanged) = 0;

protected:
	void displayCheckingForUpdates(const std::string& inDestination);

	CentralState& m_centralState;
	ExternalDependencyCache& m_dependencyCache;
};
using DependencyBuilder = Unique<IDependencyBuilder>;
}
