/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "State/Distribution/ProcessDistTarget.hpp"

#include "State/BuildState.hpp"
#include "System/Files.hpp"
#include "Utility/Path.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
ProcessDistTarget::ProcessDistTarget(const BuildState& inState) :
	IDistTarget(inState, DistTargetType::Process)
{
}

/*****************************************************************************/
bool ProcessDistTarget::initialize()
{
	Path::toUnix(m_path);

	if (!m_state.replaceVariablesInString(m_path, this))
		return false;

	if (!replaceVariablesInPathList(m_arguments))
		return false;

	return true;
}

/*****************************************************************************/
bool ProcessDistTarget::validate()
{
	if (!m_dependsOn.empty())
	{
		if (String::equals(this->name(), m_dependsOn))
		{
			Diagnostic::error("The process target '{}' depends on itself. Remove the 'dependsOn' key.", this->name());
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
			Diagnostic::error("The distribution target '{}' depends on the '{}' target which either doesn't exist or sequenced later.", this->name(), m_dependsOn);
			return false;
		}
	}

	if (!Files::pathExists(m_path))
	{
		auto resolved = Files::which(m_path);
		if (resolved.empty() && m_dependsOn.empty())
		{
			Diagnostic::error("The process path for the distribution target '{}' doesn't exist: {}", this->name(), m_path);
			return false;
		}

		m_path = std::move(resolved);
	}

	return true;
}

/*****************************************************************************/
const std::string& ProcessDistTarget::path() const noexcept
{
	return m_path;
}

void ProcessDistTarget::setPath(std::string&& inValue) noexcept
{
	m_path = std::move(inValue);
}

/*****************************************************************************/
const StringList& ProcessDistTarget::arguments() const noexcept
{
	return m_arguments;
}

void ProcessDistTarget::addArguments(StringList&& inList)
{
	List::forEach(inList, this, &ProcessDistTarget::addArgument);
}

void ProcessDistTarget::addArgument(std::string&& inValue)
{
	m_arguments.emplace_back(std::move(inValue));
}

/*****************************************************************************/
const std::string& ProcessDistTarget::dependsOn() const noexcept
{
	return m_dependsOn;
}

void ProcessDistTarget::setDependsOn(std::string&& inValue) noexcept
{
	m_dependsOn = std::move(inValue);
}
}
