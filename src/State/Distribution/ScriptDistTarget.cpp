/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "State/Distribution/ScriptDistTarget.hpp"

#include "State/BuildState.hpp"
#include "Terminal/Path.hpp"
#include "Utility/List.hpp"

namespace chalet
{
/*****************************************************************************/
ScriptDistTarget::ScriptDistTarget() :
	IDistTarget(DistTargetType::Script)
{
}

/*****************************************************************************/
void ScriptDistTarget::initialize(const BuildState& inState)
{
	const auto& targetName = this->name();
	for (auto& script : m_scripts)
	{
		inState.paths.replaceVariablesInPath(script, targetName);
	}
}

/*****************************************************************************/
bool ScriptDistTarget::validate()
{
	return true;
}

/*****************************************************************************/
const StringList& ScriptDistTarget::scripts() const noexcept
{
	return m_scripts;
}

void ScriptDistTarget::addScripts(StringList&& inList)
{
	List::forEach(inList, this, &ScriptDistTarget::addScript);
}

void ScriptDistTarget::addScript(std::string&& inValue)
{
	List::addIfDoesNotExist(m_scripts, std::move(inValue));
}

}
