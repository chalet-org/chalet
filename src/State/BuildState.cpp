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
	bundle(environment, targets, paths, compilerTools),
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
	for (auto& target : targets)
	{
		if (target->isProject())
		{
			projectNames.push_back(target->name());
		}
	}

	cache.checkIfCompileStrategyChanged();
	cache.checkIfWorkingDirectoryChanged();

	cache.removeStaleProjectCaches(info.buildConfiguration(), projectNames, BuildCache::Type::Local);
	cache.removeBuildIfCacheChanged(paths.buildOutputDir());
	cache.saveEnvironmentCache();
}

}
