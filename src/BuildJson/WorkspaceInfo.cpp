/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "BuildJson/WorkspaceInfo.hpp"

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
	m_targetArchitecture.set(m_inputs.targetArchitecture());
}

/*****************************************************************************/
const std::string& WorkspaceInfo::workspace() const noexcept
{
	return m_workspace;
}

void WorkspaceInfo::setWorkspace(const std::string& inValue) noexcept
{
	m_workspace = inValue;
}

/*****************************************************************************/
const std::string& WorkspaceInfo::version() const noexcept
{
	return m_version;
}
void WorkspaceInfo::setVersion(const std::string& inValue) noexcept
{
	m_version = inValue;
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
CpuArchitecture WorkspaceInfo::hostArchitecture() const noexcept
{
	return m_hostArchitecture.val;
}

const std::string& WorkspaceInfo::hostArchitectureString() const noexcept
{
	return m_hostArchitecture.str;
}

/*****************************************************************************/
CpuArchitecture WorkspaceInfo::targetArchitecture() const noexcept
{
	return m_targetArchitecture.val;
}

const std::string& WorkspaceInfo::targetArchitectureString() const noexcept
{
	return m_targetArchitecture.str;
}

/*****************************************************************************/
void WorkspaceInfo::Architecture::set(const std::string& inValue) noexcept
{
	// https://gcc.gnu.org/onlinedocs/gcc/x86-Options.html
	str = inValue;

	if (String::equals("x64", str))
	{
		val = CpuArchitecture::X64;
	}
	else if (String::equals("x86", str))
	{
		val = CpuArchitecture::X86;
	}
	else if (String::equals("arm", str))
	{
		val = CpuArchitecture::ARM;
	}
	else if (String::equals("arm64", str))
	{
		val = CpuArchitecture::ARM64;
	}
	else
	{
		val = CpuArchitecture::Unknown;
	}
}

}