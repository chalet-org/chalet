/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "State/BuildState.hpp"

#include "Libraries/Format.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Path.hpp"

namespace chalet
{
/*****************************************************************************/
BuildState::BuildState(const CommandLineInputs& inInputs) :
	m_platform(inInputs.platform()),
	environment(m_buildConfiguration),
	bundle(environment, projects, paths, compilers),
	cache(info, paths)
{
}

/*****************************************************************************/
void BuildState::initializeBuild(const CommandLineInputs& inInputs)
{
	paths.initialize(buildConfiguration());

	initializeCache(inInputs);
}

/*****************************************************************************/
void BuildState::initializeCache(const CommandLineInputs& inInputs)
{
	cache.initialize(inInputs.appPath());

	StringList projectNames;
	for (auto& project : projects)
	{
		projectNames.push_back(project->name());
	}

	cache.checkIfCompileStrategyChanged();
	cache.checkIfWorkingDirectoryChanged();

	cache.removeStaleProjectCaches(buildConfiguration(), projectNames, BuildCache::Type::Local);
	cache.removeBuildIfCacheChanged(paths.buildDir());
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
	chalet_assert(!m_platform.empty(), "Platform was empty");
	return m_platform;
}

}
