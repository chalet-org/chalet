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
	compilerTools(info),
	paths(m_inputs, info),
	environment(paths),
	msvcEnvironment(*this),
	bundle(environment, targets, paths, compilerTools),
	cache(info, paths)
{
}

/*****************************************************************************/
void BuildState::initializeBuild()
{
	paths.initialize();
	environment.initialize();
	for (auto& target : targets)
	{
		target->initialize();
	}

	initializeCache();
}

/*****************************************************************************/
void BuildState::initializeCache()
{
	cache.initialize(m_inputs.appPath());

	cache.checkIfCompileStrategyChanged();
	cache.checkIfWorkingDirectoryChanged();

	cache.removeStaleProjectCaches(BuildCache::Type::Local);
	cache.removeBuildIfCacheChanged(paths.buildOutputDir());
	cache.saveEnvironmentCache();
}

}
