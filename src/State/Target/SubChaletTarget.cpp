/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "State/Target/SubChaletTarget.hpp"

#include "State/BuildState.hpp"
#include "Terminal/Commands.hpp"
#include "Utility/Hash.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
SubChaletTarget::SubChaletTarget(const BuildState& inState) :
	IBuildTarget(inState, BuildTargetType::SubChalet)
{
}

/*****************************************************************************/
bool SubChaletTarget::initialize()
{
	if (!IBuildTarget::initialize())
		return false;

	if (!m_state.replaceVariablesInString(m_buildFile, this))
		return false;

	if (!m_state.replaceVariablesInString(m_location, this))
		return false;

	m_targetFolder = m_location;
	if (String::equals('.', m_targetFolder))
		m_targetFolder = this->name();

	if (m_targets.empty())
	{
		m_targets.emplace_back("all");
	}

	return true;
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

	if (m_targets.empty())
	{
		Diagnostic::error("Chalet target '{}' did not contain any targets (expected at least 'all')", targetName);
		result = false;
	}

	return result;
}

/*****************************************************************************/
std::string SubChaletTarget::getHash() const
{
	auto targets = String::join(m_targets);

	auto hashable = Hash::getHashableString(this->name(), m_location, m_targetFolder, m_buildFile, targets, m_recheck, m_rebuild, m_clean);

	return Hash::string(hashable);
}

/*****************************************************************************/
const StringList& SubChaletTarget::targets() const noexcept
{
	return m_targets;
}
void SubChaletTarget::addTargets(StringList&& inList)
{
	List::forEach(inList, this, &SubChaletTarget::addTarget);
}
void SubChaletTarget::addTarget(std::string&& inValue)
{
	List::addIfDoesNotExist(m_targets, std::move(inValue));
}

/*****************************************************************************/
const std::string& SubChaletTarget::location() const noexcept
{
	return m_location;
}

const std::string& SubChaletTarget::targetFolder() const noexcept
{
	return m_targetFolder;
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

/*****************************************************************************/
bool SubChaletTarget::rebuild() const noexcept
{
	return m_rebuild;
}
void SubChaletTarget::setRebuild(const bool inValue) noexcept
{
	m_rebuild = inValue;
}

/*****************************************************************************/
bool SubChaletTarget::clean() const noexcept
{
	return m_clean;
}
void SubChaletTarget::setClean(const bool inValue) noexcept
{
	m_clean = inValue;
}

}
