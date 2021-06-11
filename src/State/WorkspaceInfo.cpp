/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "State/WorkspaceInfo.hpp"

#include "Libraries/Format.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Path.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
WorkspaceInfo::WorkspaceInfo(const CommandLineInputs& inInputs) :
	m_inputs(inInputs)
{
	m_hostArchitecture.set(m_inputs.hostArchitecture());
	setTargetArchitecture(m_inputs.targetArchitecture());
}

/*****************************************************************************/
const std::string& WorkspaceInfo::workspace() const noexcept
{
	return m_workspace;
}

void WorkspaceInfo::setWorkspace(std::string&& inValue) noexcept
{
	m_workspace = std::move(inValue);
}

/*****************************************************************************/
const std::string& WorkspaceInfo::version() const noexcept
{
	return m_version;
}
void WorkspaceInfo::setVersion(std::string&& inValue) noexcept
{
	m_version = std::move(inValue);
}

/*****************************************************************************/
std::size_t WorkspaceInfo::hash() const noexcept
{
	return m_hash;
}
void WorkspaceInfo::setHash(const std::size_t inValue) noexcept
{
	m_hash = inValue;
}

/*****************************************************************************/
const std::string& WorkspaceInfo::buildConfiguration() const noexcept
{
	chalet_assert(!m_buildConfiguration.empty(), "Build configuration is empty");
	return m_buildConfiguration;
}

void WorkspaceInfo::setBuildConfiguration(const std::string& inValue) noexcept
{
	m_buildConfiguration = inValue;
}

/*****************************************************************************/
const std::string& WorkspaceInfo::platform() const noexcept
{
	return m_inputs.platform();
}

const StringList& WorkspaceInfo::notPlatforms() const noexcept
{
	return m_inputs.notPlatforms();
}

/*****************************************************************************/
Arch::Cpu WorkspaceInfo::hostArchitecture() const noexcept
{
	return m_hostArchitecture.val;
}

const std::string& WorkspaceInfo::hostArchitectureString() const noexcept
{
	return m_hostArchitecture.str;
}

/*****************************************************************************/
Arch::Cpu WorkspaceInfo::targetArchitecture() const noexcept
{
	return m_targetArchitecture.val;
}

const std::string& WorkspaceInfo::targetArchitectureString() const noexcept
{
	return m_targetArchitecture.str;
}

void WorkspaceInfo::setTargetArchitecture(const std::string& inValue) noexcept
{
	if (inValue.empty())
	{
		auto arch = Arch::getHostCpuArchitecture();
		m_targetArchitecture.set(arch);
	}
	else
	{
		m_targetArchitecture.set(inValue);
	}
}

}