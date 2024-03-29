/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "State/Target/ScriptBuildTarget.hpp"

#include "Core/CommandLineInputs.hpp"
#include "State/AncillaryTools.hpp"
#include "State/BuildState.hpp"
#include "System/Files.hpp"
#include "Utility/Hash.hpp"
#include "Utility/List.hpp"
#include "Utility/Path.hpp"
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

	Path::toUnix(m_file);

	if (!m_state.replaceVariablesInString(m_file, this))
		return false;

	if (!replaceVariablesInPathList(m_arguments))
		return false;

	return true;
}

/*****************************************************************************/
bool ScriptBuildTarget::validate()
{
	auto pathResult = m_state.tools.scriptAdapter().getScriptTypeFromPath(m_file, m_state.inputs.inputFile());
	if (pathResult.type == ScriptType::None)
		return false;

	m_file = std::move(pathResult.file);
	m_scriptType = pathResult.type;

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

	if (m_dependsOn.empty() && !Files::pathExists(m_file))
	{
		Diagnostic::error("File for the script target '{}' doesn't exist: {}", this->name(), m_file);
		return false;
	}

	return true;
}

/*****************************************************************************/
const std::string& ScriptBuildTarget::getHash() const
{
	if (m_hash.empty())
	{
		auto args = String::join(m_arguments);

		auto hashable = Hash::getHashableString(this->name(), m_file, args);

		m_hash = Hash::string(hashable);
	}

	return m_hash;
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
