/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "State/Target/IBuildTarget.hpp"

#include "State/BuildState.hpp"
#include "State/Target/CMakeTarget.hpp"
#include "State/Target/ProcessBuildTarget.hpp"
#include "State/Target/ScriptBuildTarget.hpp"
#include "State/Target/SourceTarget.hpp"
#include "State/Target/SubChaletTarget.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"

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
		case BuildTargetType::Project:
			return std::make_unique<SourceTarget>(inState);
		case BuildTargetType::Script:
			return std::make_unique<ScriptBuildTarget>(inState);
		case BuildTargetType::SubChalet:
			return std::make_unique<SubChaletTarget>(inState);
		case BuildTargetType::CMake:
			return std::make_unique<CMakeTarget>(inState);
		case BuildTargetType::Process:
			return std::make_unique<ProcessBuildTarget>(inState);
		default:
			break;
	}

	Diagnostic::errorAbort("Unimplemented BuildTargetType requested: ", static_cast<int>(inType));
	return nullptr;
}

/*****************************************************************************/
bool IBuildTarget::initialize()
{
	replaceVariablesInPathList(m_copyFilesOnRun);

	return true;
}

/*****************************************************************************/
void IBuildTarget::replaceVariablesInPathList(StringList& outList) const
{
	for (auto& dir : outList)
	{
		m_state.replaceVariablesInString(dir, this);
	}
}

/*****************************************************************************/
void IBuildTarget::processEachPathList(StringList&& outList, std::function<void(std::string&& inValue)> onProcess) const
{
	replaceVariablesInPathList(outList);

	StringList tmpList = std::move(outList);
	for (auto&& val : tmpList)
	{
		onProcess(std::move(val));
	}
}

/*****************************************************************************/
BuildTargetType IBuildTarget::type() const noexcept
{
	return m_type;
}
bool IBuildTarget::isSources() const noexcept
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
bool IBuildTarget::isProcess() const noexcept
{
	return m_type == BuildTargetType::Process;
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
const StringList& IBuildTarget::copyFilesOnRun() const noexcept
{
	return m_copyFilesOnRun;
}

void IBuildTarget::addCopyFilesOnRun(StringList&& inList)
{
	List::forEach(inList, this, &IBuildTarget::addCopyFileOnRun);
}

void IBuildTarget::addCopyFileOnRun(std::string&& inValue)
{
	// if (inValue.back() != '/')
	// 	inValue += '/'; // no!

	List::addIfDoesNotExist(m_copyFilesOnRun, std::move(inValue));
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
