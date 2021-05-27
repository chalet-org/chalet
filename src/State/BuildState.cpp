/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "State/BuildState.hpp"

#include "Libraries/Format.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Path.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
BuildState::BuildState(const CommandLineInputs& inInputs) :
	m_inputs(inInputs),
	environment(m_buildConfiguration),
	paths(m_inputs),
	msvcEnvironment(m_inputs, paths),
	bundle(environment, projects, paths, compilerTools),
	cache(info, paths)
{
}

/*****************************************************************************/
void BuildState::initializeBuild()
{
	paths.initialize(buildConfiguration());

	initializeCache();
}

/*****************************************************************************/
void BuildState::initializeCache()
{
	cache.initialize(m_inputs.appPath());

	StringList projectNames;
	for (auto& project : projects)
	{
		projectNames.push_back(project->name());
	}

	cache.checkIfCompileStrategyChanged();
	cache.checkIfTargetArchitectureChanged();
	cache.checkIfWorkingDirectoryChanged();

	cache.removeStaleProjectCaches(buildConfiguration(), projectNames, BuildCache::Type::Local);
	cache.removeBuildIfCacheChanged(paths.buildOutputDir());
	cache.saveEnvironmentCache();
}

/*****************************************************************************/
const std::string& BuildState::buildConfiguration() const noexcept
{
	chalet_assert(!m_buildConfiguration.empty(), "Build configuration is empty");
	return m_buildConfiguration;
}

void BuildState::setBuildConfiguration(const std::string& inValue) noexcept
{
	m_buildConfiguration = inValue;
}

/*****************************************************************************/
const std::string& BuildState::platform() const noexcept
{
	return m_inputs.platform();
}

const StringList& BuildState::notPlatforms() const noexcept
{
	return m_inputs.notPlatforms();
}

/*****************************************************************************/
CpuArchitecture BuildState::hostArchitecture() const noexcept
{
	return m_inputs.hostArchitecture();
}
CpuArchitecture BuildState::targetArchitecture() const noexcept
{
	return m_targetArchitecture;
}

void BuildState::setTargetArchitecture(const std::string& inValue) noexcept
{
	if (m_archSet)
		return;

	if (String::equals("x64", inValue))
	{
		m_targetArchitecture = CpuArchitecture::X64;
	}
	else if (String::equals("x86", inValue))
	{
		m_targetArchitecture = CpuArchitecture::X86;
	}
	else if (String::equals("arm", inValue))
	{
		m_targetArchitecture = CpuArchitecture::ARM;
	}
	else if (String::equals("arm64", inValue))
	{
		m_targetArchitecture = CpuArchitecture::ARM64;
	}
	else
	{
		m_targetArchitecture = m_inputs.hostArchitecture();
	}

	m_archSet = true;
}

}
