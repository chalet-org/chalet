/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Bundler/AppBundlerLinux.hpp"

#include "State/Distribution/BundleTarget.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Environment.hpp"
#include "Terminal/Output.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"

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
	const std::string kUserApplications{ ".local/share/applications" };

	m_home = m_state.paths.homeDirectory();

	m_applicationsPath = fmt::format("{}/{}", m_home, kUserApplications);
}

/*****************************************************************************/
bool AppBundlerLinux::removeOldFiles()
{
	auto& buildTargets = m_bundle.buildTargets();

	for (auto& target : m_state.targets)
	{
		if (target->isProject())
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
	const auto& icon = m_bundle.linuxBundle().icon();
	const auto& desktopEntry = m_bundle.linuxBundle().desktopEntry();
	if (icon.empty() || desktopEntry.empty())
		return true; // Nothing to do

	if (!getMainExecutable())
		return true; // No executable -- we don't care

	Output::lineBreak();
	Diagnostic::info("Creating the XDG Desktop Entry for the application");
	Output::lineBreak();

	const std::string bundlePath = getBundlePath();

	if (!icon.empty())
	{
		if (!Commands::copy(icon, bundlePath))
			return false;
	}

	if (!m_mainExecutable.empty())
	{
		const auto filename = fmt::format("{}/{}", bundlePath, m_mainExecutable);

		const auto desktopEntryFile = fmt::format("{}.desktop", String::getPathFolderBaseName(filename));
		const auto iconPath = fmt::format("{}/{}", bundlePath, String::getPathFilename(icon));

		if (!Commands::copyRename(desktopEntry, desktopEntryFile))
			return false;

		if (!Commands::readFileAndReplace(desktopEntryFile, [&](std::string& fileContents) {
				String::replaceAll(fileContents, "${mainExecutable}", Commands::getAbsolutePath(filename));
				String::replaceAll(fileContents, "${path}", Commands::getAbsolutePath(bundlePath));
				String::replaceAll(fileContents, "${name}", m_bundle.name());
				String::replaceAll(fileContents, "${icon}", Commands::getAbsolutePath(iconPath));
			}))
			return false;

		if (!Commands::setExecutableFlag(desktopEntryFile))
			return false;

		// TODO: Flag for this?
		if (!Environment::isContinuousIntegrationServer())
		{
			Commands::copy(desktopEntryFile, m_applicationsPath);
		}
	}

	// Output::lineBreak();

	return true;
}

/*****************************************************************************/
std::string AppBundlerLinux::getBundlePath() const
{
	return m_bundle.subDirectory();
}

/*****************************************************************************/
std::string AppBundlerLinux::getExecutablePath() const
{
	return m_bundle.subDirectory();
}

/*****************************************************************************/
std::string AppBundlerLinux::getResourcePath() const
{
	return m_bundle.subDirectory();
}

}
