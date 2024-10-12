/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

namespace chalet
{
class ExternalDependencyCache;

struct IDependencyBuilder
{
	IDependencyBuilder(ExternalDependencyCache& inDependencyCache);
	CHALET_DISALLOW_COPY_MOVE(IDependencyBuilder);
	virtual ~IDependencyBuilder() = default;

	virtual bool localPathShouldUpdate(const std::string& inDestination) = 0;

private:
	ExternalDependencyCache& m_dependencyCache;
};
}
