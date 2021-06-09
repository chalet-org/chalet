/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Bundler/AppBundlerWindows.hpp"

namespace chalet
{
/*****************************************************************************/
AppBundlerWindows::AppBundlerWindows(BuildState& inState, BundleTarget& inBundle, const bool inCleanOutput) :
	IAppBundler(inState, inBundle, inCleanOutput)
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
