/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "State/Target/SubChaletTarget.hpp"

#include "State/BuildState.hpp"
#include "Terminal/Commands.hpp"

namespace chalet
{
/*****************************************************************************/
SubChaletTarget::SubChaletTarget(const BuildState& inState) :
	IBuildTarget(inState, BuildTargetType::SubChalet)
{
}

/*****************************************************************************/
void SubChaletTarget::initialize()
{
	const auto& targetName = this->name();
	m_state.paths.replaceVariablesInPath(m_buildFile, targetName);
	m_state.paths.replaceVariablesInPath(m_location, targetName);
}

/*****************************************************************************/
bool SubChaletTarget::validate()
{
	const auto& targetName = this->name();

	bool result = true;
	if (!Commands::pathExists(m_location))
	{
		Diagnostic::error("location for Chalet target '{}' doesn't exist: {}", targetName, m_location);
		result = false;
	}

	if (!m_buildFile.empty() && !Commands::pathExists(fmt::format("{}/{}", m_location, m_buildFile)))
	{
		Diagnostic::error("buildFile '{}' for Chalet target '{}' was not found in the location: {}", m_buildFile, targetName, m_location);
		result = false;
	}

	return result;
}

/*****************************************************************************/
const std::string& SubChaletTarget::location() const noexcept
{
	return m_location;
}

void SubChaletTarget::setLocation(std::string&& inValue) noexcept
{
	m_location = std::move(inValue);
}

/*****************************************************************************/
const std::string& SubChaletTarget::buildFile() const noexcept
{
	return m_buildFile;
}

void SubChaletTarget::setBuildFile(std::string&& inValue) noexcept
{
	m_buildFile = std::move(inValue);
}

/*****************************************************************************/
bool SubChaletTarget::recheck() const noexcept
{
	return m_recheck;
}

void SubChaletTarget::setRecheck(const bool inValue) noexcept
{
	m_recheck = inValue;
}

}
