/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "State/Dependency/GitDependency.hpp"

#include "Core/CommandLineInputs.hpp"

#include "State/CentralState.hpp"
#include "Utility/Hash.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
GitDependency::GitDependency(const CentralState& inCentralState) :
	IExternalDependency(inCentralState, ExternalDependencyType::Git)
{
}

/*****************************************************************************/
bool GitDependency::initialize()
{
	if (!parseDestination())
		return false;

	return true;
}

/*****************************************************************************/
const std::string& GitDependency::getHash() const
{
	if (m_hash.empty())
	{
		auto hashable = Hash::getHashableString(this->name(), m_destination, m_repository, m_branch, m_tag, m_commit);
		m_hash = Hash::string(hashable);
	}

	return m_hash;
}

/*****************************************************************************/
bool GitDependency::validate()
{
	if (m_repository.empty())
	{
		Diagnostic::error("The git dependency repository was blank for '{}'.", this->name());
		return false;
	}

	if (m_destination.empty())
	{
		Diagnostic::error("The git dependency destination was blank for '{}'.", this->name());
		return false;
	}

	const bool hasBranch = !m_branch.empty();
	const bool hasTag = !m_tag.empty();
	const bool hasCommit = !m_commit.empty();

	if (hasTag && hasCommit && hasBranch)
	{
		Diagnostic::error("The git dependency '{}' is invalid - can't have a branch, tag and commit.", this->name());
		return false;
	}

	if (hasTag && hasCommit)
	{
		Diagnostic::error("The git dependency '{}' is invalid - can't have both a tag and commit.", this->name());
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
