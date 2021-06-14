/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_BUILD_INFO_HPP
#define CHALET_BUILD_INFO_HPP

#include "Core/Arch.hpp"
#include "Core/CommandLineInputs.hpp"

namespace chalet
{
struct BuildInfo
{
	explicit BuildInfo(const CommandLineInputs& inInputs);

	const std::string& workspace() const noexcept;
	void setWorkspace(std::string&& inValue) noexcept;

	const std::string& version() const noexcept;
	void setVersion(std::string&& inValue) noexcept;

	std::size_t hash() const noexcept;
	void setHash(const std::size_t inValue) noexcept;

	const std::string& buildConfiguration() const noexcept;
	void setBuildConfiguration(const std::string& inValue) noexcept;

	Arch::Cpu hostArchitecture() const noexcept;
	const std::string& hostArchitectureString() const noexcept;

	Arch::Cpu targetArchitecture() const noexcept;
	const std::string& targetArchitectureString() const noexcept;
	void setTargetArchitecture(const std::string& inValue) noexcept;

private:
	const CommandLineInputs& m_inputs;

	std::string m_workspace;
	std::string m_version;
	std::size_t m_hash;

	std::string m_buildConfiguration;

	Arch m_hostArchitecture;
	Arch m_targetArchitecture;
};
}

#endif // CHALET_BUILD_INFO_HPP
