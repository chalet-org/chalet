/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "State/Target/ProcessBuildTarget.hpp"

#include "State/BuildState.hpp"
#include "System/Files.hpp"
#include "Utility/Path.hpp"
#include "Utility/Hash.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
ProcessBuildTarget::ProcessBuildTarget(const BuildState& inState) :
	IBuildTarget(inState, BuildTargetType::Process)
{
}

/*****************************************************************************/
bool ProcessBuildTarget::initialize()
{
	if (!IBuildTarget::initialize())
		return false;

	Path::toUnix(m_path);

	if (!m_state.replaceVariablesInString(m_path, this))
		return false;

	if (!replaceVariablesInPathList(m_arguments))
		return false;

	return true;
}

/*****************************************************************************/
bool ProcessBuildTarget::validate()
{
	if (!m_dependsOn.empty())
	{
		if (String::equals(this->name(), m_dependsOn))
		{
			Diagnostic::error("The distribution process target '{}' depends on itself. Remove the 'dependsOn' key.", this->name());
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
			Diagnostic::error("The distribution process target '{}' depends on the '{}' target which either doesn't exist or sequenced later.", this->name(), m_dependsOn);
			return false;
		}
	}

	if (!Files::pathExists(m_path))
	{
		auto resolved = Files::which(m_path);
		if (resolved.empty() && m_dependsOn.empty())
		{
			Diagnostic::error("The process path for the target '{}' doesn't exist: {}", this->name(), m_path);
			return false;
		}

		m_path = std::move(resolved);
	}

	return true;
}

/*****************************************************************************/
std::string ProcessBuildTarget::getHash() const
{
	auto args = String::join(m_arguments);

	auto hashable = Hash::getHashableString(this->name(), m_path, args);

	return Hash::string(hashable);
}

/*****************************************************************************/
const std::string& ProcessBuildTarget::path() const noexcept
{
	return m_path;
}

void ProcessBuildTarget::setPath(std::string&& inValue) noexcept
{
	m_path = std::move(inValue);
}

/*****************************************************************************/
const StringList& ProcessBuildTarget::arguments() const noexcept
{
	return m_arguments;
}

void ProcessBuildTarget::addArguments(StringList&& inList)
{
	List::forEach(inList, this, &ProcessBuildTarget::addArgument);
}

void ProcessBuildTarget::addArgument(std::string&& inValue)
{
	m_arguments.emplace_back(std::move(inValue));
}

/*****************************************************************************/
const std::string& ProcessBuildTarget::dependsOn() const noexcept
{
	return m_dependsOn;
}

void ProcessBuildTarget::setDependsOn(std::string&& inValue) noexcept
{
	m_dependsOn = std::move(inValue);
}

}
