/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_BUILD_ENVIRONMENT_HPP
#define CHALET_BUILD_ENVIRONMENT_HPP

#include "Compile/CodeLanguage.hpp"
#include "Compile/CompilerConfig.hpp"
#include "Compile/Strategy/StrategyType.hpp"
#include "Core/CommandLineInputs.hpp"

namespace chalet
{
struct BuildPaths;
struct WorkspaceInfo;

struct BuildEnvironment
{
	explicit BuildEnvironment(const BuildPaths& inPaths);

	void initialize();

	uint processorCount() const noexcept;

	StrategyType strategy() const noexcept;
	void setStrategy(const std::string& inValue) noexcept;

	uint maxJobs() const noexcept;
	void setMaxJobs(const uint inValue) noexcept;

	bool showCommands() const noexcept;
	void setShowCommands(const bool inValue) noexcept;
	bool cleanOutput() const noexcept;

	bool dumpAssembly() const noexcept;
	void setDumpAssembly(const bool inValue) noexcept;

	const StringList& path() const noexcept;
	void addPaths(StringList&& inList);
	void addPath(std::string&& inValue);
	std::string makePathVariable(const std::string& inRootPath);

private:
	const BuildPaths& m_paths;

	std::string m_externalDepDir{ "chalet_external" };
	StringList m_path;

	std::string m_pathString;
	StringList m_pathInternal;

	uint m_processorCount = 0;
	uint m_maxJobs = 0;

	StrategyType m_strategy = StrategyType::Makefile;

	bool m_dumpAssembly = false;
	bool m_showCommands = false;
};
}

#endif // CHALET_BUILD_ENVIRONMENT_HPP
