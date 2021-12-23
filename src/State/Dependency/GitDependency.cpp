/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "State/Dependency/GitDependency.hpp"

#include "Core/CommandLineInputs.hpp"

#include "State/BuildPaths.hpp"
#include "State/CentralState.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
GitDependency::GitDependency(const CentralState& inCentralState) :
	IBuildDependency(inCentralState, BuildDependencyType::Git)
{
}

/*****************************************************************************/
bool GitDependency::validate()
{
	if (!parseDestination())
		return false;

	if (m_destination.empty() || String::startsWith('.', m_destination) || String::startsWith('/', m_destination))
	{
		// This shouldn't occur, but would be pretty bad if it did
		Diagnostic::error("The external dependency destination was blank for '{}'.", m_repository);
		return false;
	}

	return true;
}

/*****************************************************************************/
const std::string& GitDependency::repository() const noexcept
{
	return m_repository;
}

void GitDependency::setRepository(std::string&& inValue) noexcept
{
	m_repository = std::move(inValue);
}

/*****************************************************************************/
const std::string& GitDependency::branch() const noexcept
{
	return m_branch;
}

void GitDependency::setBranch(std::string&& inValue) noexcept
{
	m_branch = std::move(inValue);
}

/*****************************************************************************/
const std::string& GitDependency::tag() const noexcept
{
	return m_tag;
}

void GitDependency::setTag(std::string&& inValue) noexcept
{
	m_tag = std::move(inValue);
}

/*****************************************************************************/
const std::string& GitDependency::commit() const noexcept
{
	return m_commit;
}

void GitDependency::setCommit(std::string&& inValue) noexcept
{
	m_commit = std::move(inValue);
}

/*****************************************************************************/
const std::string& GitDependency::destination() const noexcept
{
	return m_destination;
}

/*****************************************************************************/
bool GitDependency::submodules() const noexcept
{
	return m_submodules;
}

void GitDependency::setSubmodules(const bool inValue) noexcept
{
	m_submodules = inValue;
}

/*****************************************************************************/
bool GitDependency::parseDestination()
{
	// LOG("repository: ", m_repository);
	// LOG("branch: ", m_branch);
	// LOG("tag: ", m_tag);
	// LOG("name: ", m_name);
	// LOG("destination: ", m_destination);

	if (!m_destination.empty())
		return false;

	const auto& externalDir = m_centralState.inputs().externalDirectory();
	chalet_assert(!externalDir.empty(), "externalDir can't be blank.");

	auto& targetName = this->name();

	if (!targetName.empty())
	{
		m_destination = fmt::format("{}/{}", externalDir, targetName);
		return true;
	}

	chalet_assert(!m_repository.empty(), "dependency git repository can't be blank.");

	if (!String::endsWith(".git", m_repository))
	{
		Diagnostic::error("'repository' was found but did not end with '.git'");
		return false;
	}

	std::string baseName = String::getPathBaseName(m_repository);

	m_destination = fmt::format("{}/{}", externalDir, baseName);

	// LOG(m_destination);

	return true;
}

}
