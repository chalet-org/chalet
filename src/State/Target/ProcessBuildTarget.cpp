/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "State/Target/ProcessBuildTarget.hpp"

#include "State/BuildPaths.hpp"
#include "State/BuildState.hpp"
#include "State/Target/CMakeTarget.hpp"
#include "State/Target/SourceTarget.hpp"
#include "System/Files.hpp"
#include "Utility/Hash.hpp"
#include "Utility/List.hpp"
#include "Utility/Path.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
ProcessBuildTarget::ProcessBuildTarget(const BuildState& inState) :
	IBuildTarget(inState, BuildTargetType::Process)
{
}

/*****************************************************************************/
bool ProcessBuildTarget::initialize()
{
	if (!IBuildTarget::initialize())
		return false;

	Path::toUnix(m_path);

	const auto globMessage = "Check that they exist and glob patterns can be resolved";
	if (!expandGlobPatternsInList(m_dependsOn, GlobMatch::Files))
	{
		Diagnostic::error("There was a problem resolving the files for the '{}' target. {}.", this->name(), globMessage);
		return false;
	}

	if (!m_state.replaceVariablesInString(m_path, this))
		return false;

	if (!replaceVariablesInPathList(m_arguments))
		return false;

	if (!replaceVariablesInPathList(m_dependsOn))
		return false;

	return true;
}

/*****************************************************************************/
bool ProcessBuildTarget::validate()
{
	if (!resolveDependentTargets(m_dependsOn, m_path, "dependsOn"))
		return false;

	return true;
}

/*****************************************************************************/
const std::string& ProcessBuildTarget::getHash() const
{
	if (m_hash.empty())
	{
		auto args = String::join(m_arguments);

		auto hashable = Hash::getHashableString(this->name(), m_path, args);

		m_hash = Hash::string(hashable);
	}

	return m_hash;
}

/*****************************************************************************/
const std::string& ProcessBuildTarget::path() const noexcept
{
	return m_path;
}

void ProcessBuildTarget::setPath(std::string&& inValue) noexcept
{
	m_path = std::move(inValue);
}

/*****************************************************************************/
const StringList& ProcessBuildTarget::arguments() const noexcept
{
	return m_arguments;
}

void ProcessBuildTarget::addArguments(StringList&& inList)
{
	List::forEach(inList, this, &ProcessBuildTarget::addArgument);
}

void ProcessBuildTarget::addArgument(std::string&& inValue)
{
	m_arguments.emplace_back(std::move(inValue));
}

/*****************************************************************************/
const StringList& ProcessBuildTarget::dependsOn() const noexcept
{
	return m_dependsOn;
}

void ProcessBuildTarget::addDependsOn(StringList&& inList)
{
	List::forEach(inList, this, &ProcessBuildTarget::addDependsOn);
}

void ProcessBuildTarget::addDependsOn(std::string&& inValue)
{
	m_dependsOn.emplace_back(std::move(inValue));
}
}
