/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_DEPENDENCY_GIT_HPP
#define CHALET_DEPENDENCY_GIT_HPP

#include "State/Dependency/IBuildDependency.hpp"

namespace chalet
{
struct GitDependency final : public IBuildDependency
{
	explicit GitDependency(const CentralState& inCentralState);

	virtual bool validate() final;

	const std::string& repository() const noexcept;
	void setRepository(std::string&& inValue) noexcept;

	const std::string& branch() const noexcept;
	void setBranch(std::string&& inValue) noexcept;

	const std::string& tag() const noexcept;
	void setTag(std::string&& inValue) noexcept;

	const std::string& commit() const noexcept;
	void setCommit(std::string&& inValue) noexcept;

	const std::string& destination() const noexcept;

	bool submodules() const noexcept;
	void setSubmodules(const bool inValue) noexcept;

	bool needsUpdate() const noexcept;
	void setNeedsUpdate(const bool inValue) noexcept;

private:
	bool parseDestination();

	std::string m_repository;
	std::string m_branch;
	std::string m_tag;
	std::string m_commit;
	std::string m_destination;

	bool m_submodules = false;
	bool m_needsUpdate = false;
};
}

#endif // CHALET_DEPENDENCY_GIT_HPP
