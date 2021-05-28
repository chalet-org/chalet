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
IBuildTarget::IBuildTarget(const BuildState& inState, const TargetType inType) :
	m_state(inState),
	m_type(inType)
{
}

/*****************************************************************************/
[[nodiscard]] std::unique_ptr<IBuildTarget> IBuildTarget::make(const BuildState& inState, const TargetType inType)
{
	switch (inType)
	{
		case TargetType::Project:
			return std::make_unique<ProjectTarget>(inState);
		case TargetType::Script:
			return std::make_unique<ScriptTarget>(inState);
		case TargetType::CMake:
			return std::make_unique<CMakeTarget>(inState);
		default:
			break;
	}

	Diagnostic::errorAbort(fmt::format("Unimplemented TargetType requested: ", static_cast<int>(inType)));
	return nullptr;
}

/*****************************************************************************/
TargetType IBuildTarget::type() const noexcept
{
	return m_type;
}
bool IBuildTarget::isProject() const noexcept
{
	return m_type == TargetType::Project;
}
bool IBuildTarget::isScript() const noexcept
{
	return m_type == TargetType::Script;
}
bool IBuildTarget::isCMake() const noexcept
{
	return m_type == TargetType::CMake;
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

/*****************************************************************************/
void IBuildTarget::parseStringVariables(std::string& outString)
{
	String::replaceAll(outString, "${configuration}", m_state.info.buildConfiguration());

	const auto& externalDepDir = m_state.environment.externalDepDir();
	if (!externalDepDir.empty())
	{
		String::replaceAll(outString, "${externalDepDir}", externalDepDir);
	}

	if (!name().empty())
	{
		String::replaceAll(outString, "${name}", name());
	}
}

}
