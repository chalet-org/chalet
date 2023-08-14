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
#include "Utility/Hash.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"

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

	Path::sanitize(m_file);

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

	if (!m_dependsOn.empty())
	{
		if (String::equals(this->name(), m_dependsOn))
		{
			Diagnostic::error("The script target '{}' depends on itself. Remove the 'dependsOn' key.", this->name());
			return false;
		}

		bool found = false;
		for (auto& target : m_state.targets)
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
			Diagnostic::error("The script target '{}' depends on the '{}' target which either doesn't exist or is sequenced later.", this->name(), m_dependsOn);
			return false;
		}
	}

	if (m_dependsOn.empty() && !Commands::pathExists(m_file))
	{
		Diagnostic::error("File for the script target '{}' doesn't exist: {}", this->name(), m_file);
		return false;
	}

	return true;
}

/*****************************************************************************/
std::string ScriptBuildTarget::getHash() const
{
	auto args = String::join(m_arguments);

	auto hashable = Hash::getHashableString(this->name(), m_file, args);

	return Hash::string(hashable);
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

/*****************************************************************************/
const std::string& ScriptBuildTarget::dependsOn() const noexcept
{
	return m_dependsOn;
}

void ScriptBuildTarget::setDependsOn(std::string&& inValue) noexcept
{
	m_dependsOn = std::move(inValue);
}

}
