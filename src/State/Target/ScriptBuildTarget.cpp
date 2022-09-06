/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "State/Target/ScriptBuildTarget.hpp"

#include "Core/CommandLineInputs.hpp"
#include "State/AncillaryTools.hpp"
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

	if (!m_state.replaceVariablesInString(m_file, this))
		return false;

	if (!replaceVariablesInPathList(m_arguments))
		return false;

	return true;
}

/*****************************************************************************/
bool ScriptBuildTarget::validate()
{
	auto [resolved, scriptType] = m_state.tools.scriptAdapter().getScriptTypeFromPath(m_file, m_state.inputs.inputFile());
	if (scriptType == ScriptType::None)
		return false;

	m_file = std::move(resolved);
	m_scriptType = scriptType;

	if (!Commands::pathExists(m_file))
	{
		Diagnostic::error("File for the script target '{}' doesn't exist: {}", this->name(), m_file);
		return false;
	}

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

/*****************************************************************************/
ScriptType ScriptBuildTarget::scriptType() const noexcept
{
	return m_scriptType;
}

void ScriptBuildTarget::setScriptTye(const ScriptType inType) noexcept
{
	m_scriptType = inType;
}

/*****************************************************************************/
const StringList& ScriptBuildTarget::arguments() const noexcept
{
	return m_arguments;
}

void ScriptBuildTarget::addArguments(StringList&& inList)
{
	List::forEach(inList, this, &ScriptBuildTarget::addArgument);
}

void ScriptBuildTarget::addArgument(std::string&& inValue)
{
	m_arguments.emplace_back(std::move(inValue));
}

}
