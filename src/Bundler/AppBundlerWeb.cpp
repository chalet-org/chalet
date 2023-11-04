/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Bundler/AppBundlerWeb.hpp"

#include "State/BuildPaths.hpp"
#include "State/BuildState.hpp"
#include "State/Distribution/BundleTarget.hpp"
#include "State/Target/IBuildTarget.hpp"
#include "State/Target/SourceTarget.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
AppBundlerWeb::AppBundlerWeb(BuildState& inState, const BundleTarget& inBundle, BinaryDependencyMap& inDependencyMap) :
	IAppBundler(inState, inBundle, inDependencyMap)
{
}

/*****************************************************************************/
bool AppBundlerWeb::removeOldFiles()
{
	return true;
}

/*****************************************************************************/
bool AppBundlerWeb::bundleForPlatform()
{
	const auto& buildTargets = m_bundle.buildTargets();

	StringList wasmFiles;
	StringList jsFiles;
	for (auto& target : m_state.targets)
	{
		if (target->isSources())
		{
			auto& project = static_cast<const SourceTarget&>(*target);

			if (!List::contains(buildTargets, project.name()))
				continue;

			auto outputFilePath = m_state.paths.getTargetFilename(project);
			if (project.isExecutable())
			{
				wasmFiles.emplace_back(fmt::format("{}.wasm", String::getPathFolderBaseName(outputFilePath)));
				jsFiles.emplace_back(fmt::format("{}.js", String::getPathFolderBaseName(outputFilePath)));
			}
		}
	}

	auto executablePath = getExecutablePath();
	for (auto& file : wasmFiles)
	{
		if (!copyIncludedPath(file, executablePath))
			continue;
	}
	for (auto& file : jsFiles)
	{
		if (!copyIncludedPath(file, executablePath))
			continue;
	}

	return true;
}

/*****************************************************************************/
std::string AppBundlerWeb::getBundlePath() const
{
	return m_bundle.subdirectory();
}

/*****************************************************************************/
std::string AppBundlerWeb::getExecutablePath() const
{
	return m_bundle.subdirectory();
}

/*****************************************************************************/
std::string AppBundlerWeb::getResourcePath() const
{
	return m_bundle.subdirectory();
}

/*****************************************************************************/
std::string AppBundlerWeb::getFrameworksPath() const
{
	return m_bundle.subdirectory();
}

}
