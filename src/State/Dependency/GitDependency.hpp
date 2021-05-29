/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_DEPENDENCY_GIT_HPP
#define CHALET_DEPENDENCY_GIT_HPP

#include "State/Dependency/IBuildDependency.hpp"

namespace chalet
{
struct GitDependency : public IBuildDependency
{
	explicit GitDependency(const BuildState& inState);

	const std::string& repository() const noexcept;
	void setRepository(const std::string& inValue) noexcept;

	const std::string& branch() const noexcept;
	void setBranch(const std::string& inValue) noexcept;

	const std::string& tag() const noexcept;
	void setTag(const std::string& inValue) noexcept;

	const std::string& commit() const noexcept;
	void setCommit(const std::string& inValue) noexcept;

	const std::string& destination() const noexcept;

	bool submodules() const noexcept;
	void setSubmodules(const bool inValue) noexcept;

	bool parseDestination();

private:
	std::string m_repository;
	std::string m_branch{ "master" };
	std::string m_tag;
	std::string m_commit;
	std::string m_name;
	std::string m_destination;

	bool m_submodules = false;
};
}

#endif // CHALET_DEPENDENCY_GIT_HPP
