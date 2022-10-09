/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_BUILD_INFO_HPP
#define CHALET_BUILD_INFO_HPP

#include "Core/Arch.hpp"

namespace chalet
{
struct CommandLineInputs;
struct BuildInfo
{
	explicit BuildInfo(const CommandLineInputs& inInputs);

	bool initialize();

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

	const std::string& osTarget() const noexcept;
	const std::string& osTargetVersion() const noexcept;

	uint maxJobs() const noexcept;
	bool dumpAssembly() const noexcept;
	bool generateCompileCommands() const noexcept;
	bool launchProfiler() const noexcept;
	bool keepGoing() const noexcept;

private:
	std::string m_buildConfiguration;
	std::string m_osTarget;
	std::string m_osTargetVersion;
	std::string m_hostArchTriple;

	Arch m_hostArchitecture;
	Arch m_targetArchitecture;

	uint m_maxJobs = 0;

	bool m_dumpAssembly = false;
	bool m_generateCompileCommands = false;
	bool m_launchProfiler = true;
	bool m_keepGoing = false;
};
}

#endif // CHALET_BUILD_INFO_HPP
