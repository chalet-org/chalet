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

	const std::string& buildConfiguration() const noexcept;
	const std::string& buildConfigurationNoAssert() const noexcept;
	void setBuildConfiguration(const std::string& inValue) noexcept;

	Arch::Cpu hostArchitecture() const noexcept;
	const std::string& hostArchitectureTriple() const noexcept;
	void setHostArchitecture(const std::string& inValue) noexcept;

	Arch::Cpu targetArchitecture() const noexcept;
	const std::string& targetArchitectureTriple() const noexcept;
	const std::string& targetArchitectureString() const noexcept;
	void setTargetArchitecture(const std::string& inValue) noexcept;

	const StringList& archOptions() const noexcept;
	const StringList& universalArches() const noexcept;

	uint maxJobs() const noexcept;
	bool dumpAssembly() const noexcept;
	bool generateCompileCommands() const noexcept;

private:
	const CommandLineInputs& m_inputs;

	std::string m_buildConfiguration;

	Arch m_hostArchitecture;
	Arch m_targetArchitecture;

	uint m_maxJobs = 0;
	uint m_processorCount = 0;

	bool m_dumpAssembly = false;
	bool m_generateCompileCommands = false;
};
}

#endif // CHALET_BUILD_INFO_HPP
