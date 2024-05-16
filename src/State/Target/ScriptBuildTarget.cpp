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

	bool dependsOnTargets = false;
	for (auto it = m_dependsOn.begin(); it != m_dependsOn.end();)
	{
		auto& depends = *it;
		if (Files::pathExists(depends))
		{
			it++;
			continue;
		}

		if (depends.find_first_of("/\\") != std::string::npos)
		{
			Diagnostic::error("The script target '{}' depends on a path that was not found: {}", this->name(), depends);
			return false;
		}

		if (String::equals(this->name(), depends))
		{
			Diagnostic::error("The script target '{}' depends on itself. Remove it from 'dependsOn'.", this->name());
			return false;
		}

		bool found = false;
		bool erase = true;
		for (auto& target : m_state.targets)
		{
			if (String::equals(target->name(), this->name()))
				break;

			if (String::equals(target->name(), depends))
			{
				if (target->isSources())
				{
					depends = m_state.paths.getTargetFilename(static_cast<const SourceTarget&>(*target));
					erase = !depends.empty();
				}
				else if (target->isCMake())
				{
					depends = m_state.paths.getTargetFilename(static_cast<const CMakeTarget&>(*target));
					erase = !depends.empty();
				}
				dependsOnTargets = true;
				found = true;
				break;
			}
		}
		if (!found)
		{
			Diagnostic::error("The script target '{}' depends on the '{}' target which either doesn't exist or is sequenced later.", this->name(), depends);
			return false;
		}

		if (erase)
			it = m_dependsOn.erase(it);
		else
			it++;
	}

	if (!Files::pathExists(m_file) && !dependsOnTargets)
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
}
