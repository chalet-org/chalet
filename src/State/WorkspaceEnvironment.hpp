/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_WORKSPACE_ENVIRONMENT_HPP
#define CHALET_WORKSPACE_ENVIRONMENT_HPP

namespace chalet
{
struct BuildPaths;

struct WorkspaceEnvironment
{
	WorkspaceEnvironment() = default;

	bool initialize(BuildPaths& inPaths);

	const std::string& workspaceName() const noexcept;
	void setWorkspaceName(std::string&& inValue) noexcept;

	const std::string& version() const noexcept;
	void setVersion(std::string&& inValue) noexcept;

	const StringList& searchPaths() const noexcept;
	void addSearchPaths(StringList&& inList);
	void addSearchPath(std::string&& inValue);
	std::string makePathVariable(const std::string& inRootPath) const;

private:
	StringList m_searchPaths;

	std::string m_workspaceName;
	std::string m_version;
};
}

#endif // CHALET_WORKSPACE_ENVIRONMENT_HPP
