/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Bundler/AppBundlerLinux.hpp"

#include "State/BuildPaths.hpp"
#include "State/BuildState.hpp"
#include "State/Distribution/BundleTarget.hpp"
#include "State/Target/IBuildTarget.hpp"
#include "State/Target/SourceTarget.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Output.hpp"
// #include "Terminal/Shell.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"
#include "Utility/Timer.hpp"

namespace chalet
{
/*
	// desktop entry should output:

	[Desktop Entry]
	Version=1.0
	Type=Application
	Categories=Game;Application;
	Terminal=false
	Exec=/home/user/dev/project/dist/app
	Path=/home/user/dev/project/dist
	Name=My Project
	Comment=Short Description
	Icon=/home/user/dev/project/dist/app.png
*/

/*****************************************************************************/
AppBundlerLinux::AppBundlerLinux(BuildState& inState, const BundleTarget& inBundle, BinaryDependencyMap& inDependencyMap) :
	IAppBundler(inState, inBundle, inDependencyMap)
{
	m_home = m_state.paths.homeDirectory();

	m_applicationsPath = fmt::format("{}/.local/share/applications", m_home);
}

/*****************************************************************************/
bool AppBundlerLinux::removeOldFiles()
{
	auto& buildTargets = m_bundle.buildTargets();

	for (auto& target : m_state.targets)
	{
		if (target->isSources())
		{
			auto& project = static_cast<const SourceTarget&>(*target);
			if (!List::contains(buildTargets, project.name()))
				continue;

			if (!project.isExecutable())
				continue;

			const auto outputFile = fmt::format("{}/{}.desktop", m_applicationsPath, String::getPathBaseName(project.outputFile()));

			static_cast<void>(Commands::remove(outputFile));
		}
	}

	return true;
}

/*****************************************************************************/
bool AppBundlerLinux::bundleForPlatform()
{
#if defined(CHALET_LINUX)
	if (!m_bundle.hasLinuxDesktopEntry())
		return true; // Nothing to do

	if (!getMainExecutable(m_mainExecutable))
		return true; // No executable -- we don't care

	Timer timer;

	const auto& icon = m_bundle.linuxDesktopEntryIcon();
	const auto& desktopEntry = m_bundle.linuxDesktopEntryTemplate();

	const std::string bundlePath = getBundlePath();

	if (!icon.empty())
	{
		if (!Commands::copy(icon, bundlePath))
			return false;
	}

	const auto filename = fmt::format("{}/{}", bundlePath, m_mainExecutable);

	const auto desktopEntryFile = fmt::format("{}/{}.desktop", bundlePath, m_bundle.name());
	const auto iconPath = icon.empty() ? icon : Commands::getAbsolutePath(fmt::format("{}/{}", bundlePath, String::getPathFilename(icon)));

	if (!Commands::copyRename(desktopEntry, desktopEntryFile))
		return false;

	Diagnostic::stepInfoEllipsis("Creating the XDG Desktop Entry for the application");

	if (!Commands::readFileAndReplace(desktopEntryFile, [&](std::string& fileContents) {
			String::replaceAll(fileContents, "${mainExecutable}", Commands::getAbsolutePath(filename));
			String::replaceAll(fileContents, "${path}", Commands::getAbsolutePath(bundlePath));
			String::replaceAll(fileContents, "${name}", m_bundle.name());
			String::replaceAll(fileContents, "${icon}", iconPath);
		}))
		return false;

	if (!Commands::setExecutableFlag(desktopEntryFile))
		return false;

	// TODO: Flag for this?
	/*if (!Shell::isContinuousIntegrationServer())
	{
		Commands::copy(desktopEntryFile, m_applicationsPath);
	}*/

	// Output::lineBreak();

	Diagnostic::printDone(timer.asString());

	return true;
#else
	return false;
#endif
}

/*****************************************************************************/
std::string AppBundlerLinux::getBundlePath() const
{
	return m_bundle.subdirectory();
}

/*****************************************************************************/
std::string AppBundlerLinux::getExecutablePath() const
{
	return m_bundle.subdirectory();
}

/*****************************************************************************/
std::string AppBundlerLinux::getResourcePath() const
{
	return m_bundle.subdirectory();
}

/*****************************************************************************/
std::string AppBundlerLinux::getFrameworksPath() const
{
	return m_bundle.subdirectory();
}

}
