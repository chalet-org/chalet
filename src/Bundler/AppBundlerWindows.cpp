/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Bundler/AppBundlerWindows.hpp"

#include "State/Distribution/BundleTarget.hpp"

namespace chalet
{
/*****************************************************************************/
AppBundlerWindows::AppBundlerWindows(BuildState& inState, const BundleTarget& inBundle, BinaryDependencyMap& inDependencyMap) :
	IAppBundler(inState, inBundle, inDependencyMap)
{
}

/*****************************************************************************/
bool AppBundlerWindows::removeOldFiles()
{
	return true;
}

/*****************************************************************************/
bool AppBundlerWindows::bundleForPlatform()
{
	return true;
}

/*****************************************************************************/
std::string AppBundlerWindows::getBundlePath() const
{
	return m_bundle.outDir();
}

/*****************************************************************************/
std::string AppBundlerWindows::getExecutablePath() const
{
	return m_bundle.outDir();
}

/*****************************************************************************/
std::string AppBundlerWindows::getResourcePath() const
{
	return m_bundle.outDir();
}

}
