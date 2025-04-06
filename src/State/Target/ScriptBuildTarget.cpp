/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "State/Target/ScriptBuildTarget.hpp"

#include "Core/CommandLineInputs.hpp"
#include "State/AncillaryTools.hpp"
#include "State/BuildPaths.hpp"
#include "State/BuildState.hpp"
#include "State/Target/CMakeTarget.hpp"
#include "State/Target/SourceTarget.hpp"
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

	const auto globMessage = "Check that they exist and glob patterns can be resolved";
	if (!expandGlobPatternsInList(m_dependsOn, GlobMatch::Files))
	{
		Diagnostic::error("There was a problem resolving the files for the '{}' target. {}.", this->name(), globMessage);
		return false;
	}

	if (!m_state.replaceVariablesInString(m_file, this))
		return false;

	if (!replaceVariablesInPathList(m_arguments))
		return false;

	if (!replaceVariablesInPathList(m_dependsOn))
		return false;

	if (!m_state.replaceVariablesInString(m_workingDirectory, this))
		return false;

	if (m_dependsOnSelf && !m_file.empty())
		m_dependsOn.push_back(m_file);

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

	bool result = true;
	if (!validateWorkingDirectory(m_workingDirectory))
		result = false;

	if (!resolveDependentTargets(m_dependsOn, m_file, "dependsOn"))
		result = false;

	return result;
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
const std::string& ScriptBuildTarget::workingDirectory() const noexcept
{
	return m_workingDirectory;
}
void ScriptBuildTarget::setWorkingDirectory(std::string&& inValue) noexcept
{
	m_workingDirectory = std::move(inValue);
}

/*****************************************************************************/
const StringList& ScriptBuildTarget::dependsOn() const noexcept
{
	return m_dependsOn;
}

void ScriptBuildTarget::addDependsOn(StringList&& inList)
{
	List::forEach(inList, this, &ScriptBuildTarget::addDependsOn);
}

void ScriptBuildTarget::addDependsOn(std::string&& inValue)
{
	m_dependsOn.emplace_back(std::move(inValue));
}

/*****************************************************************************/
void ScriptBuildTarget::setDependsOnSelf(const bool inValue)
{
	m_dependsOnSelf = inValue;
}
}
