/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Bundler/IAppBundler.hpp"

#include "State/BuildState.hpp"
#include "State/Distribution/BundleTarget.hpp"

#if defined(CHALET_WIN32)
	#include "Bundler/AppBundlerWindows.hpp"
#elif defined(CHALET_MACOS)
	#include "Bundler/AppBundlerMacOS.hpp"
#elif defined(CHALET_LINUX)
	#include "Bundler/AppBundlerLinux.hpp"
#endif

namespace chalet
{
/*****************************************************************************/
[[nodiscard]] std::unique_ptr<IAppBundler> IAppBundler::make(BuildState& inState, const BundleTarget& inBundle, BinaryDependencyMap& inDependencyMap, const std::string& inBuildFile)
{
#if defined(CHALET_WIN32)
	UNUSED(inBuildFile);
	return std::make_unique<AppBundlerWindows>(inState, inBundle, inDependencyMap);
#elif defined(CHALET_MACOS)
	return std::make_unique<AppBundlerMacOS>(inState, inBundle, inDependencyMap, inBuildFile);
#elif defined(CHALET_LINUX)
	UNUSED(inBuildFile);
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
}
