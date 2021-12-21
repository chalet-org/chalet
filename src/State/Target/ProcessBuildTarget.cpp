/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "State/Target/ProcessBuildTarget.hpp"

#include "State/BuildPaths.hpp"
#include "State/BuildState.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Path.hpp"
#include "Utility/List.hpp"

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

	const auto& targetName = this->name();
	m_state.paths.replaceVariablesInPath(m_path, targetName);

	return true;
}

/*****************************************************************************/
bool ProcessBuildTarget::validate()
{
	if (!Commands::pathExists(m_path))
	{
		auto resolved = Commands::which(m_path);
		if (resolved.empty())
		{
			Diagnostic::error("The process path for the target '{}' doesn't exist: {}", this->name(), m_path);
			return false;
		}

		m_path = std::move(resolved);
	}

	return true;
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

}
