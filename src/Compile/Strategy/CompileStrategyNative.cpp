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
std::string CompileStrategyNative::name() const noexcept
{
	return "Native";
}

/*****************************************************************************/
bool CompileStrategyNative::initialize()
{
	if (m_initialized)
		return false;

	const auto& cachePathId = m_state.cachePathId();

	m_cacheFolder = m_state.cache.getCachePath(cachePathId);
	m_cacheFile = fmt::format("{}/build.chalet", m_cacheFolder);

	const bool cacheExists = Files::pathExists(m_cacheFolder) && Files::pathExists(m_cacheFile);
	m_cacheNeedsUpdate = !cacheExists;

	if (!Files::pathExists(m_cacheFolder))
		Files::makeDirectory(m_cacheFolder);

	m_initialized = true;

	return true;
}

/*****************************************************************************/
bool CompileStrategyNative::addProject(const SourceTarget& inProject)
{
	if (inProject.willBuild())
	{
		const auto& name = inProject.name();
		if (!m_nativeGenerator.addProject(inProject, m_outputs.at(name), m_toolchains.at(name)))
			return false;
	}

	return ICompileStrategy::addProject(inProject);
}

/*****************************************************************************/
bool CompileStrategyNative::doPreBuild()
{
	m_nativeGenerator.initialize();
	if (m_initialized && m_cacheNeedsUpdate)
	{
		std::string contents;
		std::ofstream(m_cacheFile) << contents
								   << std::endl;
	}

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
	{
		m_filesUpdated = true;
		return false;
	}

	m_filesUpdated |= m_nativeGenerator.targetCompiled();

	return ICompileStrategy::buildProject(inProject);
}

}
