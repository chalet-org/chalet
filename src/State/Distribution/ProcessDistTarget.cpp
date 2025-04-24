/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "State/Distribution/ProcessDistTarget.hpp"

#include "State/BuildPaths.hpp"
#include "State/BuildState.hpp"
#include "State/Target/IBuildTarget.hpp"
#include "System/Files.hpp"
#include "Utility/List.hpp"
#include "Utility/Path.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
ProcessDistTarget::ProcessDistTarget(const BuildState& inState) :
	IDistTarget(inState, DistTargetType::Process)
{
}

/*****************************************************************************/
bool ProcessDistTarget::initialize()
{
	if (!IDistTarget::initialize())
		return false;

	Path::toUnix(m_path);

	if (!m_state.replaceVariablesInString(m_path, this))
		return false;

	if (!replaceVariablesInPathList(m_arguments))
		return false;

	if (!m_state.replaceVariablesInString(m_workingDirectory, this))
		return false;

	return true;
}

/*****************************************************************************/
bool ProcessDistTarget::validate()
{
	bool result = true;
	if (!resolveDependentTargets(m_dependsOn, m_path, "dependsOn"))
		result = false;

	if (!validateWorkingDirectory(m_workingDirectory))
		result = false;

	return result;
}

/*****************************************************************************/
const std::string& ProcessDistTarget::path() const noexcept
{
	return m_path;
}

void ProcessDistTarget::setPath(std::string&& inValue) noexcept
{
	m_path = std::move(inValue);
}

/*****************************************************************************/
const StringList& ProcessDistTarget::arguments() const noexcept
{
	return m_arguments;
}

void ProcessDistTarget::addArguments(StringList&& inList)
{
	List::forEach(inList, this, &ProcessDistTarget::addArgument);
}

void ProcessDistTarget::addArgument(std::string&& inValue)
{
	m_arguments.emplace_back(std::move(inValue));
}

/*****************************************************************************/
const std::string& ProcessDistTarget::workingDirectory() const noexcept
{
	return m_workingDirectory;
}
void ProcessDistTarget::setWorkingDirectory(std::string&& inValue) noexcept
{
	m_workingDirectory = std::move(inValue);
}

/*****************************************************************************/
const std::string& ProcessDistTarget::dependsOn() const noexcept
{
	return m_dependsOn;
}

void ProcessDistTarget::setDependsOn(std::string&& inValue) noexcept
{
	m_dependsOn = std::move(inValue);
}
}
