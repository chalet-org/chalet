/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/Strategy/CompileStrategyNative.hpp"

#include "Cache/WorkspaceCache.hpp"
#include "State/BuildPaths.hpp"
#include "State/BuildState.hpp"
#include "System/Files.hpp"

namespace chalet
{
/*****************************************************************************/
CompileStrategyNative::CompileStrategyNative(BuildState& inState) :
	ICompileStrategy(StrategyType::Native, inState),
	m_nativeGenerator(inState)
{
}

/*****************************************************************************/
bool CompileStrategyNative::initialize()
{
	if (m_initialized)
		return false;

	const auto& cachePathId = m_state.cachePathId();

	auto& cacheFile = m_state.cache.file();
	UNUSED(m_state.cache.getCachePath(cachePathId, CacheType::Local));

	const bool buildStrategyChanged = cacheFile.buildStrategyChanged();
	if (buildStrategyChanged)
	{
		Files::removeRecursively(m_state.paths.buildOutputDir());
	}

	m_initialized = true;

	return true;
}

/*****************************************************************************/
bool CompileStrategyNative::addProject(const SourceTarget& inProject)
{
	const auto& name = inProject.name();

	if (!m_nativeGenerator.addProject(inProject, m_outputs.at(name), m_toolchains.at(name)))
		return false;

	return ICompileStrategy::addProject(inProject);
}

/*****************************************************************************/
bool CompileStrategyNative::doPreBuild()
{
	m_nativeGenerator.initialize();
	return ICompileStrategy::doPreBuild();
}

/*****************************************************************************/
bool CompileStrategyNative::doPostBuild() const
{
	m_nativeGenerator.dispose();
	return ICompileStrategy::doPostBuild();
}

/*****************************************************************************/
bool CompileStrategyNative::buildProject(const SourceTarget& inProject)
{
	if (!m_nativeGenerator.buildProject(inProject))
		return false;

	return true;
}

}
