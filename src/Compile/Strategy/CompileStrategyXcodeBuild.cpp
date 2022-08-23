/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/Strategy/CompileStrategyXcodeBuild.hpp"

#include "Cache/WorkspaceCache.hpp"
#include "Core/Arch.hpp"
#include "Core/CommandLineInputs.hpp"
#include "Export/IProjectExporter.hpp"
#include "State/BuildConfiguration.hpp"
#include "State/BuildInfo.hpp"
#include "State/BuildPaths.hpp"
#include "State/BuildState.hpp"
#include "State/CompilerTools.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Output.hpp"

namespace chalet
{
/*****************************************************************************/
CompileStrategyXcodeBuild::CompileStrategyXcodeBuild(BuildState& inState) :
	ICompileStrategy(StrategyType::XcodeBuild, inState)
{
}

/*****************************************************************************/
bool CompileStrategyXcodeBuild::initialize()
{
	if (m_initialized)
		return false;

	auto& cacheFile = m_state.cache.file();
	const auto& uniqueId = m_state.uniqueId();
	UNUSED(m_state.cache.getCachePath(uniqueId, CacheType::Local));

	const bool buildStrategyChanged = cacheFile.buildStrategyChanged(m_state.toolchain.strategy());
	if (buildStrategyChanged)
	{
		Commands::removeRecursively(m_state.paths.buildOutputDir());
	}

	m_initialized = true;

	return true;
}

/*****************************************************************************/
bool CompileStrategyXcodeBuild::addProject(const SourceTarget& inProject)
{
	return ICompileStrategy::addProject(inProject);
}

/*****************************************************************************/
bool CompileStrategyXcodeBuild::doFullBuild()
{
	LOG("Hello XcodeBuild");
	return true;
}

/*****************************************************************************/
bool CompileStrategyXcodeBuild::buildProject(const SourceTarget& inProject)
{
	UNUSED(inProject);
	return true;
}

}
