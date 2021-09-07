/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_WORKSPACE_ENVIRONMENT_HPP
#define CHALET_WORKSPACE_ENVIRONMENT_HPP

#include "Compile/CodeLanguage.hpp"
#include "Compile/CompilerConfig.hpp"
#include "Core/CommandLineInputs.hpp"

namespace chalet
{
struct BuildPaths;
struct BuildInfo;

struct WorkspaceEnvironment
{
	WorkspaceEnvironment() = default;

	bool initialize(BuildPaths& inPaths);

	const std::string& workspace() const noexcept;
	void setWorkspace(std::string&& inValue) noexcept;

	const std::string& version() const noexcept;
	void setVersion(std::string&& inValue) noexcept;

	const StringList& searchPaths() const noexcept;
	void addSearchPaths(StringList&& inList);
	void addSearchPath(std::string&& inValue);
	std::string makePathVariable(const std::string& inRootPath) const;

private:
	mutable StringList m_searchPaths;

	std::string m_workspace;
	std::string m_version;

	std::string m_pathString;
	StringList m_pathInternal;
};
}

#endif // CHALET_WORKSPACE_ENVIRONMENT_HPP
