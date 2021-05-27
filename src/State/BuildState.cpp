/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "State/BuildState.hpp"

namespace chalet
{
/*****************************************************************************/
BuildState::BuildState(const CommandLineInputs& inInputs) :
	m_inputs(inInputs),
	info(m_inputs),
	environment(info),
	paths(m_inputs),
	msvcEnvironment(*this),
	bundle(environment, projects, paths, compilerTools),
	cache(info, paths)
{
}

/*****************************************************************************/
void BuildState::initializeBuild()
{
	paths.initialize(info.buildConfiguration());

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

	cache.removeStaleProjectCaches(info.buildConfiguration(), projectNames, BuildCache::Type::Local);
	cache.removeBuildIfCacheChanged(paths.buildOutputDir());
	cache.saveEnvironmentCache();
}

}
