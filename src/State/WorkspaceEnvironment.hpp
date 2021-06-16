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
	WorkspaceEnvironment();

	void initialize(BuildPaths& inPaths);

	uint processorCount() const noexcept;

	uint maxJobs() const noexcept;
	void setMaxJobs(const uint inValue) noexcept;

	bool dumpAssembly() const noexcept;
	void setDumpAssembly(const bool inValue) noexcept;

	const StringList& path() const noexcept;
	void addPaths(StringList&& inList);
	void addPath(std::string&& inValue);
	std::string makePathVariable(const std::string& inRootPath) const;

private:
	std::string m_externalDepDir{ "chalet_external" };
	mutable StringList m_path;

	std::string m_pathString;
	StringList m_pathInternal;

	uint m_processorCount = 0;
	uint m_maxJobs = 0;

	bool m_dumpAssembly = false;
};
}

#endif // CHALET_WORKSPACE_ENVIRONMENT_HPP
