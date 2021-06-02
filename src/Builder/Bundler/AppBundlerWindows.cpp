/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Builder/Bundler/AppBundlerWindows.hpp"

namespace chalet
{
/*****************************************************************************/
AppBundlerWindows::AppBundlerWindows(BuildState& inState) :
	m_state(inState)
{
	UNUSED(m_state);
}

/*****************************************************************************/
bool AppBundlerWindows::removeOldFiles(const BundleTarget& bundle, const bool inCleanOutput)
{
	UNUSED(bundle, inCleanOutput);

	return true;
}

/*****************************************************************************/
bool AppBundlerWindows::bundleForPlatform(const BundleTarget& bundle, const bool inCleanOutput)
{
	UNUSED(bundle, inCleanOutput);
	return true;
}

/*****************************************************************************/
std::string AppBundlerWindows::getBundlePath(const BundleTarget& bundle) const
{
	return bundle.outDir();
}

/*****************************************************************************/
std::string AppBundlerWindows::getExecutablePath(const BundleTarget& bundle) const
{
	return bundle.outDir();
}

/*****************************************************************************/
std::string AppBundlerWindows::getResourcePath(const BundleTarget& bundle) const
{
	return bundle.outDir();
}

}
