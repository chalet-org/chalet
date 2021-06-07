/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "State/Target/ScriptTarget.hpp"

#include "State/BuildState.hpp"
#include "Terminal/Path.hpp"
#include "Utility/List.hpp"

namespace chalet
{
/*****************************************************************************/
ScriptTarget::ScriptTarget(const BuildState& inState) :
	IBuildTarget(inState, BuildTargetType::Script)
{
}

/*****************************************************************************/
void ScriptTarget::initialize()
{
	const auto& targetName = this->name();
	for (auto& script : m_scripts)
	{
		m_state.paths.replaceVariablesInPath(script, targetName);
	}
}

/*****************************************************************************/
bool ScriptTarget::validate()
{
	return true;
}

/*****************************************************************************/
const StringList& ScriptTarget::scripts() const noexcept
{
	return m_scripts;
}

void ScriptTarget::addScripts(StringList& inList)
{
	List::forEach(inList, this, &ScriptTarget::addScript);
}

void ScriptTarget::addScript(std::string& inValue)
{
	List::addIfDoesNotExist(m_scripts, std::move(inValue));
}

}
