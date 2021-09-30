/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "State/Target/ScriptBuildTarget.hpp"

#include "State/BuildState.hpp"
#include "Terminal/Path.hpp"
#include "Utility/List.hpp"

namespace chalet
{
/*****************************************************************************/
ScriptBuildTarget::ScriptBuildTarget(const BuildState& inState) :
	IBuildTarget(inState, BuildTargetType::Script)
{
}

/*****************************************************************************/
bool ScriptBuildTarget::initialize()
{
	const auto& targetName = this->name();
	for (auto& script : m_scripts)
	{
		m_state.paths.replaceVariablesInPath(script, targetName);
	}

	return true;
}

/*****************************************************************************/
bool ScriptBuildTarget::validate()
{
	return true;
}

/*****************************************************************************/
const StringList& ScriptBuildTarget::scripts() const noexcept
{
	return m_scripts;
}

void ScriptBuildTarget::addScripts(StringList&& inList)
{
	List::forEach(inList, this, &ScriptBuildTarget::addScript);
}

void ScriptBuildTarget::addScript(std::string&& inValue)
{
	List::addIfDoesNotExist(m_scripts, std::move(inValue));
}

/*****************************************************************************/
const StringList& ScriptBuildTarget::runArguments() const noexcept
{
	return m_runArguments;
}
void ScriptBuildTarget::addRunArguments(StringList&& inList)
{
	List::forEach(inList, this, &ScriptBuildTarget::addRunArgument);
}
void ScriptBuildTarget::addRunArgument(std::string&& inValue)
{
	m_runArguments.emplace_back(std::move(inValue));
}

/*****************************************************************************/
bool ScriptBuildTarget::runTarget() const noexcept
{
	return m_runTarget;
}
void ScriptBuildTarget::setRunTarget(const bool inValue) noexcept
{
	m_runTarget = inValue;
}

}
