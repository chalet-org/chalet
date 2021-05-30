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
	compilerTools(m_inputs, info),
	paths(m_inputs, info),
	environment(paths),
	msvcEnvironment(*this),
	bundle(environment, targets, paths, compilerTools),
	cache(info, paths)
{
}

/*****************************************************************************/
bool BuildState::initializeBuild()
{
	paths.initialize();
	environment.initialize();
	for (auto& target : targets)
	{
		target->initialize();
	}

	if (!compilerTools.initialize())
	{
		const auto& targetArch = compilerTools.detectedToolchain() == ToolchainType::GNU ?
			m_inputs.targetArchitecture() :
			info.targetArchitectureString();

		Diagnostic::error(fmt::format("Requested arch '{}' is not supported.", targetArch));
		return false;
	}

	initializeCache();

	return true;
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
