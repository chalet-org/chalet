/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "State/BuildInfo.hpp"

#include "Libraries/Format.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Path.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
BuildInfo::BuildInfo(const CommandLineInputs& inInputs) :
	m_inputs(inInputs)
{
	m_hostArchitecture.set(m_inputs.hostArchitecture());
	setTargetArchitecture(m_inputs.targetArchitecture());
}

/*****************************************************************************/
const std::string& BuildInfo::workspace() const noexcept
{
	return m_workspace;
}

void BuildInfo::setWorkspace(std::string&& inValue) noexcept
{
	m_workspace = std::move(inValue);
}

/*****************************************************************************/
const std::string& BuildInfo::version() const noexcept
{
	return m_version;
}
void BuildInfo::setVersion(std::string&& inValue) noexcept
{
	m_version = std::move(inValue);
}

/*****************************************************************************/
std::size_t BuildInfo::hash() const noexcept
{
	return m_hash;
}
void BuildInfo::setHash(const std::size_t inValue) noexcept
{
	m_hash = inValue;
}

/*****************************************************************************/
const std::string& BuildInfo::buildConfiguration() const noexcept
{
	chalet_assert(!m_buildConfiguration.empty(), "Build configuration is empty");
	return m_buildConfiguration;
}

void BuildInfo::setBuildConfiguration(const std::string& inValue) noexcept
{
	m_buildConfiguration = inValue;
}

/*****************************************************************************/
Arch::Cpu BuildInfo::hostArchitecture() const noexcept
{
	return m_hostArchitecture.val;
}

const std::string& BuildInfo::hostArchitectureString() const noexcept
{
	return m_hostArchitecture.str;
}

/*****************************************************************************/
Arch::Cpu BuildInfo::targetArchitecture() const noexcept
{
	return m_targetArchitecture.val;
}

const std::string& BuildInfo::targetArchitectureString() const noexcept
{
	return m_targetArchitecture.str;
}

void BuildInfo::setTargetArchitecture(const std::string& inValue) noexcept
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