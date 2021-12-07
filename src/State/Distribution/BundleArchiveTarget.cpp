/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "State/Distribution/BundleArchiveTarget.hpp"

#include "State/BuildPaths.hpp"
#include "State/BuildState.hpp"
#include "Terminal/Commands.hpp"
#include "Utility/GlobMatch.hpp"
#include "Utility/List.hpp"

namespace chalet
{
/*****************************************************************************/
BundleArchiveTarget::BundleArchiveTarget() :
	IDistTarget(DistTargetType::BundleArchive)
{
}

/*****************************************************************************/
bool BundleArchiveTarget::validate()
{
	return true;
}

/*****************************************************************************/
const StringList& BundleArchiveTarget::includes() const noexcept
{
	return m_includes;
}

void BundleArchiveTarget::addIncludes(StringList&& inList)
{
	List::forEach(inList, this, &BundleArchiveTarget::addInclude);
}

void BundleArchiveTarget::addInclude(std::string&& inValue)
{
	List::addIfDoesNotExist(m_includes, std::move(inValue));
}

}
