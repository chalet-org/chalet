/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Dependencies/IDependencyBuilder.hpp"

#include "Cache/ExternalDependencyCache.hpp"

namespace chalet
{
/*****************************************************************************/
IDependencyBuilder::IDependencyBuilder(ExternalDependencyCache& inDependencyCache) :
	m_dependencyCache(inDependencyCache)
{
}

}
