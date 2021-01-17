/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "BuildJson/DependencyGit.hpp"

#include "Libraries/Format.hpp"
#include "State/CommandLineInputs.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
DependencyGit::DependencyGit(const BuildEnvironment& inEnvironment) :
	m_environment(inEnvironment)
{
}

/*****************************************************************************/
const std::string& DependencyGit::repository() const noexcept
{
	return m_repository;
}

void DependencyGit::setRepository(const std::string& inValue) noexcept
{
	m_repository = inValue;
}

/*****************************************************************************/
const std::string& DependencyGit::branch() const noexcept
{
	return m_branch;
}

void DependencyGit::setBranch(const std::string& inValue) noexcept
{
	m_branch = inValue;
}

/*****************************************************************************/
const std::string& DependencyGit::tag() const noexcept
{
	return m_tag;
}

void DependencyGit::setTag(const std::string& inValue) noexcept
{
	m_tag = inValue;
}

/*****************************************************************************/
const std::string& DependencyGit::commit() const noexcept
{
	return m_commit;
}

void DependencyGit::setCommit(const std::string& inValue) noexcept
{
	m_commit = inValue;
}

/*****************************************************************************/
const std::string& DependencyGit::name() const noexcept
{
	return m_name;
}
void DependencyGit::setName(const std::string& inValue) noexcept
{
	if (String::startsWith(".", inValue) || String::startsWith("_", inValue) || String::startsWith("-", inValue) || String::startsWith("+", inValue))
		return;

	m_name = inValue;
}

/*****************************************************************************/
const std::string& DependencyGit::destination() const noexcept
{
	return m_destination;
}

/*****************************************************************************/
bool DependencyGit::submodules() const noexcept
{
	return m_submodules;
}

void DependencyGit::setSubmodules(const bool inValue) noexcept
{
	m_submodules = inValue;
}

/*****************************************************************************/
bool DependencyGit::parseDestination()
{
	// LOG("repository: ", m_repository);
	// LOG("branch: ", m_branch);
	// LOG("tag: ", m_tag);
	// LOG("name: ", m_name);
	// LOG("destination: ", m_destination);

	if (!m_destination.empty())
		return false;

	const auto& modulePath = m_environment.modulePath();
	chalet_assert(!modulePath.empty(), "modulePath can't be blank.");

	if (!m_name.empty())
	{
		m_destination = fmt::format("{}/{}", modulePath, m_name);
		return true;
	}

	chalet_assert(!m_repository.empty(), "dependency git repository can't be blank.");

	if (!String::endsWith(".git", m_repository))
	{
		Diagnostic::errorAbort(fmt::format("{}: 'repository' was found but did not end with '.git'", CommandLineInputs::file()));
		return false;
	}

	std::string baseName = String::getPathBaseName(m_repository);

	m_destination = fmt::format("{}/{}", modulePath, baseName);

	// LOG(m_destination);

	return true;
}

}
