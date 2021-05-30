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

/*****************************************************************************/
bool BuildState::validateState()
{
	bool result = true;
	{
		auto workingDirectory = Commands::getWorkingDirectory();
		Path::sanitize(workingDirectory, true);

		if (paths.workingDirectory() != workingDirectory)
		{
			if (!Commands::changeWorkingDirectory(paths.workingDirectory()))
			{
				Diagnostic::error(fmt::format("Error changing directory to '{}'", paths.workingDirectory()));
				result = false;
			}
		}
	}

	result &= bundle.validate();

	for (auto& target : targets)
	{
		if (target->isCMake())
		{
			tools.fetchCmakeVersion();
		}

		result &= target->validate();
	}

	auto strategy = environment.strategy();
	if (strategy == StrategyType::Makefile)
	{
		const auto& makeExec = tools.make();
		if (makeExec.empty() || !Commands::pathExists(makeExec))
		{
			Diagnostic::error(fmt::format("{} was either not defined in the cache, or not found. Aborting.", makeExec.empty() ? "make" : makeExec));
			result = false;
		}
	}
	else if (strategy == StrategyType::Ninja)
	{
		auto& ninjaExec = tools.ninja();
		if (ninjaExec.empty() || !Commands::pathExists(ninjaExec))
		{
			Diagnostic::error(fmt::format("{} was either not defined in the cache, or not found. Aborting.", ninjaExec.empty() ? "ninja" : ninjaExec));
			result = false;
		}
	}

	return result;
}

}
