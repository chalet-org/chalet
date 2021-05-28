/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_WORKSPACE_INFO_HPP
#define CHALET_WORKSPACE_INFO_HPP

#include "State/CommandLineInputs.hpp"

namespace chalet
{
struct WorkspaceInfo
{
	explicit WorkspaceInfo(const CommandLineInputs& inInputs);

	const std::string& workspace() const noexcept;
	void setWorkspace(const std::string& inValue) noexcept;

	const std::string& version() const noexcept;
	void setVersion(const std::string& inValue) noexcept;

	std::size_t hash() const noexcept;
	void setHash(const std::size_t inValue) noexcept;

	const std::string& buildConfiguration() const noexcept;
	void setBuildConfiguration(const std::string& inValue) noexcept;

	const std::string& platform() const noexcept;
	const StringList& notPlatforms() const noexcept;

	CpuArchitecture hostArchitecture() const noexcept;
	const std::string& hostArchitectureString() const noexcept;

	CpuArchitecture targetArchitecture() const noexcept;
	const std::string& targetArchitectureString() const noexcept;
	void setTargetArchitecture(const std::string& inValue) noexcept;

private:
	struct Architecture
	{
		std::string str;
		CpuArchitecture val = CpuArchitecture::Unknown;

		void set(const std::string& inValue) noexcept;
	};

	const CommandLineInputs& m_inputs;

	std::string m_workspace;
	std::string m_version;
	std::size_t m_hash;

	std::string m_buildConfiguration;

	Architecture m_hostArchitecture;
	Architecture m_targetArchitecture;
};
}

#endif // CHALET_WORKSPACE_INFO_HPP
