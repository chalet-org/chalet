/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include "Core/Arch.hpp"

namespace chalet
{
struct CommandLineInputs;
struct PlatformDependencyManager;
class BuildState;

struct BuildInfo
{
	BuildInfo(const BuildState& inState, const CommandLineInputs& inInputs);
	CHALET_DISALLOW_COPY_MOVE(BuildInfo);
	~BuildInfo();

	bool initialize();
	bool validate();

	void addRequiredPlatformDependency(const std::string& inKind, std::string&& inValue);
	void addRequiredPlatformDependency(const std::string& inKind, StringList&& inValue);

	const std::string& buildConfiguration() const noexcept;
	const std::string& buildConfigurationNoAssert() const noexcept;
	void setBuildConfiguration(const std::string& inValue) noexcept;

	Arch::Cpu hostArchitecture() const noexcept;
	const std::string& hostArchitectureTriple() const noexcept;
	const std::string& hostArchitectureString() const noexcept;
	void setHostArchitecture(const std::string& inValue) noexcept;

	Arch::Cpu targetArchitecture() const noexcept;
	const std::string& targetArchitectureTriple() const noexcept;
	const std::string& targetArchitectureString() const noexcept;
	const std::string& targetArchitectureTripleSuffix() const noexcept;
	void setTargetArchitecture(const std::string& inValue) noexcept;
	bool targettingMinGW() const;

	uint maxJobs() const noexcept;
	bool dumpAssembly() const noexcept;
	bool generateCompileCommands() const noexcept;
	bool launchProfiler() const noexcept;
	bool keepGoing() const noexcept;
	bool onlyRequired() const noexcept;

private:
	const BuildState& m_state;
	const CommandLineInputs& m_inputs;

	Unique<PlatformDependencyManager> m_platformDeps;

	std::string m_buildConfiguration;
	std::string m_hostArchTriple;

	Arch m_hostArchitecture;
	Arch m_targetArchitecture;

	uint m_maxJobs = 0;

	bool m_dumpAssembly = false;
	bool m_generateCompileCommands = false;
	bool m_launchProfiler = true;
	bool m_keepGoing = false;
	bool m_onlyRequired = false;
};
}
