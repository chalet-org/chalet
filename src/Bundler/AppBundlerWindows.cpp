/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Bundler/AppBundlerWindows.hpp"

#include "State/AncillaryTools.hpp"
#include "State/Distribution/BundleTarget.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Environment.hpp"
#include "Terminal/Output.hpp"
#include "Utility/Timer.hpp"

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
	if (!createWindowsInstaller())
		return false;

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

/*****************************************************************************/
bool AppBundlerWindows::createWindowsInstaller() const
{
	// TODO: ignore on CI for now
	//   plugins will need to be defined as well as the script, and then the plugins
	//   will need to be copied to the NSIS install directory (it's that old school)
	//
	if (Environment::isContinuousIntegrationServer())
		return true;

	const auto& nsisScript = m_bundle.windowsBundle().nsisScript();
	if (!nsisScript.empty())
	{
		Output::lineBreak();

		Timer timer;
		Diagnostic::info("Creating the Windows installer executable", Output::showCommands());

		StringList cmd{ m_state.tools.makeNsis() };
		cmd.push_back("/WX");
		cmd.push_back("/V3");
		cmd.push_back("/NOCD");
		cmd.push_back(nsisScript);

		bool result = false;
		if (Output::showCommands())
			result = Commands::subprocess(cmd);
		else
			result = Commands::subprocess(cmd, PipeOption::Close, PipeOption::StdOut);

		if (!result)
		{
			Diagnostic::error("NSIS Installer failed to compile: {}", nsisScript);
			return false;
		}

		Diagnostic::printDone(timer.asString());
	}

	return true;
}

}
