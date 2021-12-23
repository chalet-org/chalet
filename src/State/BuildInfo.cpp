/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "State/BuildInfo.hpp"

#include <thread>

#include "Core/CommandLineInputs.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Path.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
BuildInfo::BuildInfo(const CommandLineInputs& inInputs) :
	m_processorCount(std::thread::hardware_concurrency())
{
	m_hostArchitecture.set(inInputs.hostArchitecture());
	setTargetArchitecture(inInputs.targetArchitecture());

	if (inInputs.maxJobs().has_value())
		m_maxJobs = *inInputs.maxJobs();

	if (inInputs.dumpAssembly().has_value())
		m_dumpAssembly = *inInputs.dumpAssembly();

	if (inInputs.generateCompileCommands().has_value())
		m_generateCompileCommands = *inInputs.generateCompileCommands();

	if (inInputs.launchProfiler().has_value())
		m_launchProfiler = *inInputs.launchProfiler();
}

/*****************************************************************************/
bool BuildInfo::initialize()
{
#if defined(CHALET_MACOS)
	std::string macosVersion;
	auto triple = String::split(m_targetArchitecture.triple, '-');
	if (triple.size() == 3)
	{
		auto& sys = triple.back();
		sys = String::toLowerCase(sys);

		for (auto& target : StringList{ "macos", "ios", "watchos", "tvos" })
		{
			if (String::startsWith(target, sys))
			{
				m_osTarget = target;
				m_osTargetVersion = sys.substr(target.size());
				break;
			}
		}
	}
#endif

	return true;
}

/*****************************************************************************/
const std::string& BuildInfo::buildConfiguration() const noexcept
{
	chalet_assert(!m_buildConfiguration.empty(), "Build configuration is empty");
	return m_buildConfiguration;
}

const std::string& BuildInfo::buildConfigurationNoAssert() const noexcept
{
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

void BuildInfo::setHostArchitecture(const std::string& inValue) noexcept
{
	if (!inValue.empty())
	{
		m_hostArchitecture.set(inValue);
	}
}

/*****************************************************************************/
Arch::Cpu BuildInfo::targetArchitecture() const noexcept
{
	return m_targetArchitecture.val;
}

const std::string& BuildInfo::targetArchitectureTriple() const noexcept
{
	return m_targetArchitecture.triple;
}

const std::string& BuildInfo::targetArchitectureString() const noexcept
{
	return m_targetArchitecture.str;
}

const std::string& BuildInfo::targetArchitectureTripleSuffix() const noexcept
{
	return m_targetArchitecture.suffix;
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

/*****************************************************************************/
const std::string& BuildInfo::osTarget() const noexcept
{
	return m_osTarget;
}

const std::string& BuildInfo::osTargetVersion() const noexcept
{
	return m_osTargetVersion;
}

/*****************************************************************************/
uint BuildInfo::maxJobs() const noexcept
{
	return m_maxJobs;
}

/*****************************************************************************/
bool BuildInfo::dumpAssembly() const noexcept
{
	return m_dumpAssembly;
}

/*****************************************************************************/
bool BuildInfo::generateCompileCommands() const noexcept
{
	return m_generateCompileCommands;
}

/*****************************************************************************/
bool BuildInfo::launchProfiler() const noexcept
{
	return m_launchProfiler;
}

}