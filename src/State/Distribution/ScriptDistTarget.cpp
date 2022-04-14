/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "State/Distribution/ScriptDistTarget.hpp"

#include "State/BuildState.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Path.hpp"
#include "Utility/List.hpp"

namespace chalet
{
/*****************************************************************************/
ScriptDistTarget::ScriptDistTarget(const BuildState& inState) :
	IDistTarget(inState, DistTargetType::Script)
{
}

/*****************************************************************************/
bool ScriptDistTarget::initialize()
{
	m_state.replaceVariablesInString(m_file, this);

	replaceVariablesInPathList(m_arguments);

	return true;
}

/*****************************************************************************/
bool ScriptDistTarget::validate()
{
	const auto& targetName = this->name();

	if (!Commands::pathExists(m_file))
	{
		Diagnostic::error("File for the script target '{}' doesn't exist: {}", targetName, m_file);
		return false;
	}

	return true;
}

/*****************************************************************************/
const std::string& ScriptDistTarget::file() const noexcept
{
	return m_file;
}

void ScriptDistTarget::setFile(std::string&& inValue) noexcept
{
	m_file = std::move(inValue);
}

/*****************************************************************************/
const StringList& ScriptDistTarget::arguments() const noexcept
{
	return m_arguments;
}

void ScriptDistTarget::addArguments(StringList&& inList)
{
	List::forEach(inList, this, &ScriptDistTarget::addArgument);
}

void ScriptDistTarget::addArgument(std::string&& inValue)
{
	m_arguments.emplace_back(std::move(inValue));
}

}
