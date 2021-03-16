/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_BUILD_ENVIRONMENT_HPP
#define CHALET_BUILD_ENVIRONMENT_HPP

#include "BuildJson/WorkspaceInfo.hpp"
#include "Compile/CodeLanguage.hpp"
#include "Compile/CompilerConfig.hpp"
#include "Compile/Strategy/StrategyType.hpp"
#include "State/CommandLineInputs.hpp"

namespace chalet
{
struct BuildEnvironment
{
	explicit BuildEnvironment(const std::string& inBuildConfig);

	uint processorCount() const noexcept;

	StrategyType strategy() const noexcept;
	void setStrategy(const std::string& inValue) noexcept;

	const std::string& platform() const noexcept;
	void setPlatform(const std::string& inValue) noexcept;

	const std::string& externalDepDir() const noexcept;
	void setExternalDepDir(const std::string& inValue) noexcept;

	uint maxJobs() const noexcept;
	void setMaxJobs(const uint inValue) noexcept;

	bool showCommands() const noexcept;
	void setShowCommands(const bool inValue) noexcept;
	bool cleanOutput() const noexcept;

	const StringList& path() const noexcept;
	void addPaths(StringList& inList);
	void addPath(std::string& inValue);
	std::string makePathVariable(const std::string& inRootPath);

private:
	const std::string& m_buildConfiguration;

	std::string m_platform = "auto";
	std::string m_externalDepDir{ "chalet_external" };
	StringList m_path;

	std::string m_pathString;
	StringList m_pathInternal;

	uint m_processorCount = 0;
	uint m_maxJobs = 0;

	StrategyType m_strategy = StrategyType::Makefile;

	bool m_showCommands = false;
};
}

#endif // CHALET_BUILD_ENVIRONMENT_HPP
