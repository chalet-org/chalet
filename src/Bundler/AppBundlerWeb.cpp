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
	auto buildTargets = m_bundle.getRequiredBuildTargets();

	StringList wasmFiles;
	StringList jsFiles;
	for (auto& project : buildTargets)
	{
		auto outputFilePath = m_state.paths.getTargetFilename(*project);
		if (project->isExecutable())
		{
			auto noExtension = String::getPathFolderBaseName(outputFilePath);
			wasmFiles.emplace_back(fmt::format("{}.wasm", noExtension));
			jsFiles.emplace_back(fmt::format("{}.js", noExtension));
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
}
