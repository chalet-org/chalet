/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Builder/Bundler/AppBundlerLinux.hpp"

#include "Libraries/Format.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Environment.hpp"
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
	Exec=/home/user/dev/project/build/app
	Path=/home/user/dev/project/build
	Name=My Project
	Comment=Short Description
	Icon=/home/user/dev/project/build/app.png
*/

/*****************************************************************************/
AppBundlerLinux::AppBundlerLinux(BuildState& inState) :
	m_state(inState)
{
	const std::string kUserApplications = ".local/share/applications";

	auto home = Environment::getUserDirectory();
	m_home = fs::path{ home };

	m_applicationsPath = fs::path{ m_home / kUserApplications }.string();
}

/*****************************************************************************/
bool AppBundlerLinux::removeOldFiles(const bool inCleanOutput)
{
	auto& bundle = m_state.bundle;
	auto& bundleProjects = bundle.projects();

	for (auto& project : m_state.projects)
	{
		if (!project->includeInBuild())
			continue;

		if (!List::contains(bundleProjects, project->name()))
			continue;

		if (!project->isExecutable())
			continue;

		const auto& filename = fs::path{ project->outputFile() }.stem().string();
		std::string outputFile = fmt::format("{}/{}.desktop", m_applicationsPath, filename);

		static_cast<void>(Commands::remove(outputFile, inCleanOutput));
	}

	return true;
}

/*****************************************************************************/
bool AppBundlerLinux::bundleForPlatform(const bool inCleanOutput)
{
	auto& bundle = m_state.bundle;

	const auto& icon = bundle.linuxBundle().icon();
	if (icon.empty())
		return false;

	const auto& desktopEntry = bundle.linuxBundle().desktopEntry();
	const std::string bundlePath = getBundlePath();

	UNUSED(inCleanOutput, desktopEntry);

	bool result = true;
	result &= Commands::copy(icon, bundlePath, inCleanOutput);

	fs::path desktopEntryPath{ desktopEntry };
	auto& bundleProjects = bundle.projects();

	// TODO: Right now this does this for every executable, but shares the same icon
	//  Will need to rework how to make multiple bundles or something...
	//  (or just use the runProject, but that might not be desireable)
	for (auto& project : m_state.projects)
	{
		if (!project->includeInBuild())
			continue;

		if (!List::contains(bundleProjects, project->name()))
			continue;

		if (!project->isExecutable())
			continue;

		const std::string filename = fmt::format("{}/{}", bundlePath, project->outputFile());
		fs::path outDesktopEntry{ bundlePath / fs::path{ fs::path{ filename }.stem().string() + ".desktop" } };
		std::string desktopEntryString = outDesktopEntry.string();
		fs::path iconPath = bundlePath / fs::path{ icon }.filename();

		result &= Commands::copyRename(desktopEntry, desktopEntryString, inCleanOutput);

		result &= Commands::readFileAndReplace(outDesktopEntry, [&](std::string& fileContents) {
			String::replaceAll(fileContents, "${mainProject}", fs::absolute(filename).string());
			String::replaceAll(fileContents, "${path}", fs::absolute(bundlePath).string());
			String::replaceAll(fileContents, "${appName}", bundle.appName());
			String::replaceAll(fileContents, "${shortDescription}", bundle.shortDescription());
			String::replaceAll(fileContents, "${icon}", fs::absolute(iconPath).string());

			String::replaceAll(fileContents, '\\', '/');
		});

		result &= Commands::setExecutableFlag(filename, inCleanOutput);
		result &= Commands::setExecutableFlag(desktopEntryString, inCleanOutput);

		// TODO: Flag for this?
		Commands::copy(desktopEntryString, m_applicationsPath, inCleanOutput);
		break;
	}

	return result;
}

/*****************************************************************************/
std::string AppBundlerLinux::getBundlePath() const
{
	return m_state.bundle.path();
}

std::string AppBundlerLinux::getExecutablePath() const
{
	return m_state.bundle.path();
}

std::string AppBundlerLinux::getResourcePath() const
{
	return m_state.bundle.path();
}

}
