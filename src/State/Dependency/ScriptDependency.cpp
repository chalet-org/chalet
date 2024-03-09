/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "State/Dependency/ScriptDependency.hpp"

#include "Core/CommandLineInputs.hpp"
#include "State/AncillaryTools.hpp"
#include "State/BuildConfiguration.hpp"
#include "State/BuildState.hpp"
#include "State/CentralState.hpp"
#include "System/Files.hpp"
#include "Utility/Hash.hpp"
#include "Utility/List.hpp"
#include "Utility/Path.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
ScriptDependency::ScriptDependency(const CentralState& inCentralState) :
	IExternalDependency(inCentralState, ExternalDependencyType::Script)
{
}

/*****************************************************************************/
bool ScriptDependency::initialize()
{
	Path::toUnix(m_file);

	if (!m_centralState.replaceVariablesInString(m_file, this))
		return false;

	if (!replaceVariablesInPathList(m_arguments))
		return false;

	return true;
}

/*****************************************************************************/
bool ScriptDependency::validate()
{
	const auto& targetName = this->name();

	auto pathResult = m_centralState.tools.scriptAdapter().getScriptTypeFromPath(m_file, m_centralState.inputs().inputFile());
	if (pathResult.type == ScriptType::None)
		return false;

	m_file = std::move(pathResult.file);
	m_scriptType = pathResult.type;

	if (!Files::pathExists(m_file))
	{
		Diagnostic::error("File for the script target '{}' doesn't exist: {}", targetName, m_file);
		return false;
	}

	return true;
}

/*****************************************************************************/
std::string ScriptDependency::getStateHash(const BuildState& inState) const
{
	auto configHash = inState.configuration.getHash();
	auto arguments = String::join(m_arguments);
	auto hashable = Hash::getHashableString(this->name(), m_file, arguments, configHash);
	return Hash::string(hashable);
}

/*****************************************************************************/
const std::string& ScriptDependency::file() const noexcept
{
	return m_file;
}

void ScriptDependency::setFile(std::string&& inValue) noexcept
{
	m_file = std::move(inValue);
}

/*****************************************************************************/
ScriptType ScriptDependency::scriptType() const noexcept
{
	return m_scriptType;
}

void ScriptDependency::setScriptTye(const ScriptType inType) noexcept
{
	m_scriptType = inType;
}

/*****************************************************************************/
const StringList& ScriptDependency::arguments() const noexcept
{
	return m_arguments;
}

void ScriptDependency::addArguments(StringList&& inList)
{
	List::forEach(inList, this, &ScriptDependency::addArgument);
}

void ScriptDependency::addArgument(std::string&& inValue)
{
	m_arguments.emplace_back(std::move(inValue));
}

}
