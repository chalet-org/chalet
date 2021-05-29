/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "State/Target/IBuildTarget.hpp"

#include "State/BuildState.hpp"
#include "State/Target/CMakeTarget.hpp"
#include "State/Target/ProjectTarget.hpp"
#include "State/Target/ScriptTarget.hpp"
#include "Utility/String.hpp"

namespace chalet
{
IBuildTarget::IBuildTarget(const BuildState& inState, const BuildTargetType inType) :
	m_state(inState),
	m_type(inType)
{
}

/*****************************************************************************/
[[nodiscard]] std::unique_ptr<IBuildTarget> IBuildTarget::make(const BuildState& inState, const BuildTargetType inType)
{
	switch (inType)
	{
		case BuildTargetType::Project:
			return std::make_unique<ProjectTarget>(inState);
		case BuildTargetType::Script:
			return std::make_unique<ScriptTarget>(inState);
		case BuildTargetType::CMake:
			return std::make_unique<CMakeTarget>(inState);
		default:
			break;
	}

	Diagnostic::errorAbort(fmt::format("Unimplemented BuildTargetType requested: ", static_cast<int>(inType)));
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
void IBuildTarget::setDescription(const std::string& inValue) noexcept
{
	m_description = inValue;
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
