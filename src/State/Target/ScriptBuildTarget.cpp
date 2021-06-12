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
void ScriptBuildTarget::initialize()
{
	const auto& targetName = this->name();
	for (auto& script : m_scripts)
	{
		m_state.paths.replaceVariablesInPath(script, targetName);
	}
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

}
