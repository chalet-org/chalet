/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Bundler/IAppBundler.hpp"

#include "BuildEnvironment/IBuildEnvironment.hpp"
#include "Core/CommandLineInputs.hpp"
#include "State/BuildState.hpp"
#include "State/Distribution/BundleTarget.hpp"
#include "State/Target/IBuildTarget.hpp"
#include "State/Target/SourceTarget.hpp"
#include "System/Files.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"

#if defined(CHALET_WIN32)
	#include "Bundler/AppBundlerWindows.hpp"
#elif defined(CHALET_MACOS)
	#include "Bundler/AppBundlerMacOS.hpp"
#elif defined(CHALET_LINUX)
	#include "Bundler/AppBundlerLinux.hpp"
#endif
#include "Bundler/AppBundlerWeb.hpp"

namespace chalet
{
/*****************************************************************************/
[[nodiscard]] Unique<IAppBundler> IAppBundler::make(BuildState& inState, const BundleTarget& inBundle, BinaryDependencyMap& inDependencyMap)
{
	if (inState.environment->isEmscripten())
	{
		return std::make_unique<AppBundlerWeb>(inState, inBundle, inDependencyMap);
	}
#if defined(CHALET_WIN32)
	return std::make_unique<AppBundlerWindows>(inState, inBundle, inDependencyMap);
#elif defined(CHALET_MACOS)
	return std::make_unique<AppBundlerMacOS>(inState, inBundle, inDependencyMap);
#elif defined(CHALET_LINUX)
	return std::make_unique<AppBundlerLinux>(inState, inBundle, inDependencyMap);
#else
	Diagnostic::errorAbort("Unimplemented AppBundler requested: ");
	return nullptr;
#endif
}

/*****************************************************************************/
IAppBundler::IAppBundler(BuildState& inState, const BundleTarget& inBundle, BinaryDependencyMap& inDependencyMap) :
	m_state(inState),
	m_bundle(inBundle),
	m_dependencyMap(inDependencyMap)
{
}

/*****************************************************************************/
const BundleTarget& IAppBundler::bundle() const noexcept
{
	return m_bundle;
}

/*****************************************************************************/
bool IAppBundler::initialize(const std::string& inOutputDir)
{
	UNUSED(inOutputDir);
	return true;
}

/*****************************************************************************/
bool IAppBundler::quickBundleForPlatform()
{
	return false;
}

/*****************************************************************************/
StringList IAppBundler::getAllExecutables() const
{
	StringList ret;

	auto buildTargets = m_bundle.getRequiredBuildTargets();
	for (auto& project : buildTargets)
	{
		if (!project->isExecutable())
			continue;

		ret.emplace_back(project->outputFile());
	}

	return ret;
}

/*****************************************************************************/
bool IAppBundler::copyIncludedPath(const std::string& inDep, const std::string& inOutPath)
{
	return Files::copyIfDoesNotExistWithoutPrintingWorkingDirectory(inDep, inOutPath, workingDirectoryWithTrailingPathSeparator());
}

/*****************************************************************************/
const std::string& IAppBundler::workingDirectoryWithTrailingPathSeparator()
{
	if (m_cwd.empty())
	{
		m_cwd = m_state.inputs.workingDirectory() + '/';
	}
	return m_cwd;
}

/*****************************************************************************/
std::string IAppBundler::getBundlePath() const
{
	return m_bundle.subdirectory();
}

/*****************************************************************************/
std::string IAppBundler::getExecutablePath() const
{
	return m_bundle.subdirectory();
}

/*****************************************************************************/
std::string IAppBundler::getResourcePath() const
{
	return m_bundle.subdirectory();
}

/*****************************************************************************/
std::string IAppBundler::getFrameworksPath() const
{
	return m_bundle.subdirectory();
}

}
