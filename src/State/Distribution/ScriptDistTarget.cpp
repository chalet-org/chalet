/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "State/Distribution/ScriptDistTarget.hpp"

#include "Core/CommandLineInputs.hpp"
#include "State/AncillaryTools.hpp"
#include "State/BuildState.hpp"
#include "System/Files.hpp"
#include "Utility/Path.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"

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
	Path::toUnix(m_file);

	if (!m_state.replaceVariablesInString(m_file, this))
		return false;

	if (!replaceVariablesInPathList(m_arguments))
		return false;

	return true;
}

/*****************************************************************************/
bool ScriptDistTarget::validate()
{
	const auto& targetName = this->name();

	auto [resolved, scriptType] = m_state.tools.scriptAdapter().getScriptTypeFromPath(m_file, m_state.inputs.inputFile());
	if (scriptType == ScriptType::None)
		return false;

	m_file = std::move(resolved);
	m_scriptType = scriptType;

	if (!m_dependsOn.empty())
	{
		if (String::equals(this->name(), m_dependsOn))
		{
			Diagnostic::error("The distribution script target '{}' depends on itself. Remove the 'dependsOn' key.", this->name());
			return false;
		}

		bool found = false;
		for (auto& target : m_state.distribution)
		{
			if (String::equals(target->name(), this->name()))
				break;

			if (String::equals(target->name(), m_dependsOn))
			{
				found = true;
				break;
			}
		}
		if (!found)
		{
			Diagnostic::error("The distribution script target '{}' depends on the '{}' target which either doesn't exist or sequenced later.", this->name(), m_dependsOn);
			return false;
		}
	}

	if (m_dependsOn.empty() && !Files::pathExists(m_file))
	{
		Diagnostic::error("File for the distribution script target '{}' doesn't exist: {}", targetName, m_file);
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
ScriptType ScriptDistTarget::scriptType() const noexcept
{
	return m_scriptType;
}

void ScriptDistTarget::setScriptTye(const ScriptType inType) noexcept
{
	m_scriptType = inType;
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

/*****************************************************************************/
const std::string& ScriptDistTarget::dependsOn() const noexcept
{
	return m_dependsOn;
}

void ScriptDistTarget::setDependsOn(std::string&& inValue) noexcept
{
	m_dependsOn = std::move(inValue);
}

}
