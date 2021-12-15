/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "State/Target/ScriptBuildTarget.hpp"

#include "State/BuildPaths.hpp"
#include "State/BuildState.hpp"
#include "Terminal/Commands.hpp"
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
	if (!IBuildTarget::initialize())
		return false;

	const auto& targetName = this->name();
	m_state.paths.replaceVariablesInPath(m_file, targetName);

	if (!Commands::pathExists(m_file))
	{
		Diagnostic::error("File for the script target '{}' doesn't exist: {}", targetName, m_file);
		return false;
	}

	return true;
}

/*****************************************************************************/
bool ScriptBuildTarget::validate()
{
	return true;
}

/*****************************************************************************/
const std::string& ScriptBuildTarget::file() const noexcept
{
	return m_file;
}

void ScriptBuildTarget::setFile(std::string&& inValue) noexcept
{
	m_file = std::move(inValue);
}

}
