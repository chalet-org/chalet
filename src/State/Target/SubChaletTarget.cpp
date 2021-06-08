/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "State/Target/SubChaletTarget.hpp"

#include "State/BuildState.hpp"

namespace chalet
{
/*****************************************************************************/
SubChaletTarget::SubChaletTarget(const BuildState& inState) :
	IBuildTarget(inState, BuildTargetType::SubChalet)
{
}

/*****************************************************************************/
void SubChaletTarget::initialize()
{
	const auto& targetName = this->name();
	m_state.paths.replaceVariablesInPath(m_location, targetName);
}

/*****************************************************************************/
bool SubChaletTarget::validate()
{
	return true;
}

/*****************************************************************************/
const std::string& SubChaletTarget::location() const noexcept
{
	return m_location;
}

void SubChaletTarget::setLocation(std::string&& inValue) noexcept
{
	m_location = std::move(inValue);
}

/*****************************************************************************/
bool SubChaletTarget::recheck() const noexcept
{
	return m_recheck;
}

void SubChaletTarget::setRecheck(const bool inValue) noexcept
{
	m_recheck = inValue;
}

}
