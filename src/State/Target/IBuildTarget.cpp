/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "State/Target/IBuildTarget.hpp"

#include "State/BuildState.hpp"
#include "State/Target/CMakeTarget.hpp"
#include "State/Target/ScriptBuildTarget.hpp"
#include "State/Target/SourceTarget.hpp"
#include "State/Target/SubChaletTarget.hpp"
#include "Utility/String.hpp"

namespace chalet
{
IBuildTarget::IBuildTarget(const BuildState& inState, const BuildTargetType inType) :
	m_state(inState),
	m_type(inType)
{
}

/*****************************************************************************/
[[nodiscard]] BuildTarget IBuildTarget::make(const BuildTargetType inType, BuildState& inState)
{
	switch (inType)
	{
		case BuildTargetType::Project:
			return std::make_unique<SourceTarget>(inState);
		case BuildTargetType::Script:
			return std::make_unique<ScriptBuildTarget>(inState);
		case BuildTargetType::SubChalet:
			return std::make_unique<SubChaletTarget>(inState);
		case BuildTargetType::CMake:
			return std::make_unique<CMakeTarget>(inState);
		default:
			break;
	}

	Diagnostic::errorAbort("Unimplemented BuildTargetType requested for makeBuild: ", static_cast<int>(inType));
	return nullptr;
}

/*****************************************************************************/
BuildTargetType IBuildTarget::type() const noexcept
{
	return m_type;
}
bool IBuildTarget::isProject() const noexcept
{
	return m_type == BuildTargetType::Project;
}
bool IBuildTarget::isScript() const noexcept
{
	return m_type == BuildTargetType::Script;
}
bool IBuildTarget::isSubChalet() const noexcept
{
	return m_type == BuildTargetType::SubChalet;
}
bool IBuildTarget::isCMake() const noexcept
{
	return m_type == BuildTargetType::CMake;
}
/*****************************************************************************/
const std::string& IBuildTarget::name() const noexcept
{
	return m_name;
}
void IBuildTarget::setName(const std::string& inValue) noexcept
{
	m_name = inValue;
}

/*****************************************************************************/
const std::string& IBuildTarget::description() const noexcept
{
	return m_description;
}
void IBuildTarget::setDescription(std::string&& inValue) noexcept
{
	m_description = std::move(inValue);
}

/*****************************************************************************/
bool IBuildTarget::includeInBuild() const noexcept
{
	return m_includeInBuild;
}

void IBuildTarget::setIncludeInBuild(const bool inValue)
{
	m_includeInBuild &= inValue;
}
}
