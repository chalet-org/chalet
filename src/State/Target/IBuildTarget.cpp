/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "State/Target/IBuildTarget.hpp"

#include "State/BuildState.hpp"
#include "State/CompilerTools.hpp"
#include "State/WorkspaceEnvironment.hpp"
#include "Terminal/Commands.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"

#include "State/Target/CMakeTarget.hpp"
#include "State/Target/ProcessBuildTarget.hpp"
#include "State/Target/ScriptBuildTarget.hpp"
#include "State/Target/SourceTarget.hpp"
#include "State/Target/SubChaletTarget.hpp"
#include "State/Target/ValidationBuildTarget.hpp"

namespace chalet
{
IBuildTarget::IBuildTarget(const BuildState& inState, const BuildTargetType inType) :
	m_state(inState),
	m_type(inType)
{
}

/*****************************************************************************/
[[nodiscard]] BuildTarget IBuildTarget::make(const BuildTargetType inType, const BuildState& inState)
{
	switch (inType)
	{
		case BuildTargetType::Source:
			return std::make_unique<SourceTarget>(inState);
		case BuildTargetType::Script:
			return std::make_unique<ScriptBuildTarget>(inState);
		case BuildTargetType::SubChalet:
			return std::make_unique<SubChaletTarget>(inState);
		case BuildTargetType::CMake:
			return std::make_unique<CMakeTarget>(inState);
		case BuildTargetType::Process:
			return std::make_unique<ProcessBuildTarget>(inState);
		case BuildTargetType::Validation:
			return std::make_unique<ValidationBuildTarget>(inState);
		default:
			break;
	}

	Diagnostic::errorAbort("Unimplemented BuildTargetType requested: {}", static_cast<i32>(inType));
	return nullptr;
}

/*****************************************************************************/
bool IBuildTarget::initialize()
{
	return true;
}

/*****************************************************************************/
bool IBuildTarget::replaceVariablesInPathList(StringList& outList) const
{
	for (auto& dir : outList)
	{
		if (!m_state.replaceVariablesInString(dir, this))
			return false;
	}

	return true;
}

/*****************************************************************************/
bool IBuildTarget::processEachPathList(StringList inList, std::function<bool(std::string&& inValue)> onProcess) const
{
	if (!replaceVariablesInPathList(inList))
		return false;

	for (auto&& val : inList)
	{
		if (!onProcess(std::move(val)))
			return false;
	}

	return true;
}

/*****************************************************************************/
BuildTargetType IBuildTarget::type() const noexcept
{
	return m_type;
}
bool IBuildTarget::isSources() const noexcept
{
	return m_type == BuildTargetType::Source;
}
bool IBuildTarget::isSubChalet() const noexcept
{
	return m_type == BuildTargetType::SubChalet;
}
bool IBuildTarget::isCMake() const noexcept
{
	return m_type == BuildTargetType::CMake;
}
bool IBuildTarget::isScript() const noexcept
{
	return m_type == BuildTargetType::Script;
}
bool IBuildTarget::isProcess() const noexcept
{
	return m_type == BuildTargetType::Process;
}
bool IBuildTarget::isValidation() const noexcept
{
	return m_type == BuildTargetType::Validation;
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
const std::string& IBuildTarget::outputDescription() const noexcept
{
	return m_outputDescription;
}
void IBuildTarget::setOutputDescription(std::string&& inValue) noexcept
{
	m_outputDescription = std::move(inValue);
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
