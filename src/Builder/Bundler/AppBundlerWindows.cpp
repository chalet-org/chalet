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
}

/*****************************************************************************/
bool AppBundlerWindows::removeOldFiles(const bool inCleanOutput)
{
	UNUSED(inCleanOutput);

	return true;
}

/*****************************************************************************/
bool AppBundlerWindows::bundleForPlatform(const bool inCleanOutput)
{
	UNUSED(inCleanOutput);
	return true;
}

/*****************************************************************************/
std::string AppBundlerWindows::getBundlePath() const
{
	return m_state.bundle.outDir();
}

std::string AppBundlerWindows::getExecutablePath() const
{
	return m_state.bundle.outDir();
}

std::string AppBundlerWindows::getResourcePath() const
{
	return m_state.bundle.outDir();
}

}
