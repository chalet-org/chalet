/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_WORKSPACE_ENVIRONMENT_HPP
#define CHALET_WORKSPACE_ENVIRONMENT_HPP

#include "Utility/Version.hpp"

namespace chalet
{
class BuildState;

struct WorkspaceEnvironment
{
	WorkspaceEnvironment() = default;

	bool initialize(const BuildState& inPaths);

	const std::string& workspaceName() const noexcept;
	void setWorkspaceName(std::string&& inValue) noexcept;

	const std::string& versionString() const noexcept;
	void setVersion(std::string&& inValue) noexcept;
	const Version& version() const noexcept;

	const StringList& searchPaths() const noexcept;
	void addSearchPaths(StringList&& inList);
	void addSearchPath(std::string&& inValue);
	std::string makePathVariable(const std::string& inRootPath) const;
	std::string makePathVariable(const std::string& inRootPath, const StringList& inAdditionalPaths) const;

private:
	StringList m_searchPaths;

	std::string m_workspaceName;
	std::string m_versionString;

	Version m_version;
};
}

#endif // CHALET_WORKSPACE_ENVIRONMENT_HPP
