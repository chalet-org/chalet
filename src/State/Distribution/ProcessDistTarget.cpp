/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "State/Distribution/ProcessDistTarget.hpp"

#include "State/CentralState.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Path.hpp"
#include "Utility/List.hpp"

namespace chalet
{
/*****************************************************************************/
ProcessDistTarget::ProcessDistTarget(const CentralState& inCentralState) :
	IDistTarget(inCentralState, DistTargetType::Process)
{
}

/*****************************************************************************/
bool ProcessDistTarget::initialize()
{
	m_centralState.replaceVariablesInPath(m_path, this);

	replaceVariablesInPathList(m_arguments);

	return true;
}

/*****************************************************************************/
bool ProcessDistTarget::validate()
{
	if (!Commands::pathExists(m_path))
	{
		auto resolved = Commands::which(m_path);
		if (resolved.empty())
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

}
