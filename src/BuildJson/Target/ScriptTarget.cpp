/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "BuildJson/Target/ScriptTarget.hpp"

#include "State/BuildState.hpp"
#include "Terminal/Path.hpp"
#include "Utility/List.hpp"

namespace chalet
{
/*****************************************************************************/
ScriptTarget::ScriptTarget(const BuildState& inState) :
	IBuildTarget(inState, TargetType::Script)
{
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
	parseStringVariables(inValue);
	Path::sanitize(inValue);

	List::addIfDoesNotExist(m_scripts, std::move(inValue));
}

}
