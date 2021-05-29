/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_DEPENDENCY_GIT_HPP
#define CHALET_DEPENDENCY_GIT_HPP

namespace chalet
{
struct BuildPaths;

struct DependencyGit
{
	explicit DependencyGit(const BuildPaths& inPaths, const std::string& inBuildFile);

	const std::string& repository() const noexcept;
	void setRepository(const std::string& inValue) noexcept;

	const std::string& branch() const noexcept;
	void setBranch(const std::string& inValue) noexcept;

	const std::string& tag() const noexcept;
	void setTag(const std::string& inValue) noexcept;

	const std::string& commit() const noexcept;
	void setCommit(const std::string& inValue) noexcept;

	const std::string& name() const noexcept;
	void setName(const std::string& inValue) noexcept;

	const std::string& destination() const noexcept;

	bool submodules() const noexcept;
	void setSubmodules(const bool inValue) noexcept;

	bool parseDestination();

private:
	const BuildPaths& m_paths;
	const std::string& m_buildFile;

	std::string m_repository;
	std::string m_branch{ "master" };
	std::string m_tag;
	std::string m_commit;
	std::string m_name;
	std::string m_destination;

	bool m_submodules = false;
};

using DependencyList = std::vector<std::unique_ptr<DependencyGit>>;
}

#endif // CHALET_DEPENDENCY_GIT_HPP
