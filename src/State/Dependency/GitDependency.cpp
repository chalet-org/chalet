/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "State/Dependency/GitDependency.hpp"

#include "Libraries/Format.hpp"
#include "State/BuildPaths.hpp"
#include "State/BuildState.hpp"
#include "State/CommandLineInputs.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
GitDependency::GitDependency(const BuildState& inState) :
	IBuildDependency(inState, BuildDependencyType::Git)
{
}

/*****************************************************************************/
const std::string& GitDependency::repository() const noexcept
{
	return m_repository;
}

void GitDependency::setRepository(const std::string& inValue) noexcept
{
	m_repository = inValue;
}

/*****************************************************************************/
const std::string& GitDependency::branch() const noexcept
{
	return m_branch;
}

void GitDependency::setBranch(const std::string& inValue) noexcept
{
	m_branch = inValue;
}

/*****************************************************************************/
const std::string& GitDependency::tag() const noexcept
{
	return m_tag;
}

void GitDependency::setTag(const std::string& inValue) noexcept
{
	m_tag = inValue;
}

/*****************************************************************************/
const std::string& GitDependency::commit() const noexcept
{
	return m_commit;
}

void GitDependency::setCommit(const std::string& inValue) noexcept
{
	m_commit = inValue;
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

	const auto& externalDepDir = m_state.paths.externalDepDir();
	chalet_assert(!externalDepDir.empty(), "externalDepDir can't be blank.");

	if (!m_name.empty())
	{
		m_destination = fmt::format("{}/{}", externalDepDir, m_name);
		return true;
	}

	chalet_assert(!m_repository.empty(), "dependency git repository can't be blank.");

	if (!String::endsWith(".git", m_repository))
	{
		Diagnostic::errorAbort("'repository' was found but did not end with '.git'");
		return false;
	}

	std::string baseName = String::getPathBaseName(m_repository);

	m_destination = fmt::format("{}/{}", externalDepDir, baseName);

	// LOG(m_destination);

	return true;
}

}
