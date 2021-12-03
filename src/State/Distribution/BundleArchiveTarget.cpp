/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "State/Distribution/BundleArchiveTarget.hpp"

#include "State/BuildPaths.hpp"
#include "State/BuildState.hpp"
#include "Utility/List.hpp"

namespace chalet
{
/*****************************************************************************/
BundleArchiveTarget::BundleArchiveTarget() :
	IDistTarget(DistTargetType::BundleArchive)
{
}

/*****************************************************************************/
bool BundleArchiveTarget::initialize(const BuildState& inState)
{
	UNUSED(inState);
	for (auto& target : m_targets)
	{
		LOG("target:", target);
	}

	return true;
}

/*****************************************************************************/
bool BundleArchiveTarget::validate()
{
	return true;
}

/*****************************************************************************/
const StringList& BundleArchiveTarget::targets() const noexcept
{
	return m_targets;
}

void BundleArchiveTarget::addTargets(StringList&& inList)
{
	List::forEach(inList, this, &BundleArchiveTarget::addTarget);
}

void BundleArchiveTarget::addTarget(std::string&& inValue)
{
	List::addIfDoesNotExist(m_targets, std::move(inValue));
}

}
