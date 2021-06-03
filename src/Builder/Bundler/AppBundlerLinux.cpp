/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Builder/Bundler/AppBundlerLinux.hpp"

#include "Libraries/Format.hpp"
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
AppBundlerLinux::AppBundlerLinux(BuildState& inState, BundleTarget& inBundle, const bool inCleanOutput) :
	IAppBundler(inState, inBundle, inCleanOutput)
{
	const std::string kUserApplications{ ".local/share/applications" };

	auto home = Environment::getUserDirectory();
	m_home = fs::path{ home };

	m_applicationsPath = fs::path{ m_home / kUserApplications }.string();
}

/*****************************************************************************/
bool AppBundlerLinux::removeOldFiles()
{
	auto& bundleProjects = m_bundle.projects();

	for (auto& target : m_state.targets)
	{
		if (target->isProject())
		{
			auto& project = static_cast<const ProjectTarget&>(*target);
			if (!List::contains(bundleProjects, project.name()))
				continue;

			if (!project.isExecutable())
				continue;

			const auto& filename = fs::path{ project.outputFile() }.stem().string();
			std::string outputFile = fmt::format("{}/{}.desktop", m_applicationsPath, filename);

			static_cast<void>(Commands::remove(outputFile, m_cleanOutput));
		}
	}

	return true;
}

/*****************************************************************************/
bool AppBundlerLinux::bundleForPlatform()
{
	const auto& icon = m_bundle.linuxBundle().icon();
	if (icon.empty())
		return false;

	const auto& desktopEntry = m_bundle.linuxBundle().desktopEntry();
	const std::string bundlePath = getBundlePath();

	UNUSED(m_cleanOutput, desktopEntry);

	bool result = true;
	result &= Commands::copy(icon, bundlePath, m_cleanOutput);

	fs::path desktopEntryPath{ desktopEntry };
	auto& bundleProjects = m_bundle.projects();
	auto& mainProject = m_bundle.mainProject();

	// Match mainProject if defined, otherwise get first executable

	for (auto& target : m_state.targets)
	{
		if (target->isProject())
		{
			auto& project = static_cast<const ProjectTarget&>(*target);
			if (!List::contains(bundleProjects, project.name()))
				continue;

			if (!project.isExecutable())
				continue;

			if (!mainProject.empty() && !String::equals(mainProject, project.name()))
				continue;

			m_mainExecutable = project.outputFile();
			break;
		}
	}

	if (!m_mainExecutable.empty())
	{
		const auto filename = fmt::format("{}/{}", bundlePath, m_mainExecutable);
		fs::path outDesktopEntry{ bundlePath / fs::path{ fs::path{ filename }.stem().string() + ".desktop" } };
		std::string desktopEntryString = outDesktopEntry.string();
		fs::path iconPath = bundlePath / fs::path{ icon }.filename();

		result &= Commands::copyRename(desktopEntry, desktopEntryString, m_cleanOutput);

		result &= Commands::readFileAndReplace(outDesktopEntry, [&](std::string& fileContents) {
			String::replaceAll(fileContents, "${mainProject}", fs::absolute(filename).string());
			String::replaceAll(fileContents, "${path}", fs::absolute(bundlePath).string());
			String::replaceAll(fileContents, "${name}", m_bundle.name());
			String::replaceAll(fileContents, "${description}", m_bundle.description());
			String::replaceAll(fileContents, "${icon}", fs::absolute(iconPath).string());

			String::replaceAll(fileContents, '\\', '/');
		});

		result &= Commands::setExecutableFlag(filename, m_cleanOutput);
		result &= Commands::setExecutableFlag(desktopEntryString, m_cleanOutput);

		// TODO: Flag for this?
		if (!Environment::isContinuousIntegrationServer())
		{
			Commands::copy(desktopEntryString, m_applicationsPath, m_cleanOutput);
		}
	}

	Output::lineBreak();

	return result;
}

/*****************************************************************************/
std::string AppBundlerLinux::getBundlePath() const
{
	return m_bundle.outDir();
}

/*****************************************************************************/
std::string AppBundlerLinux::getExecutablePath() const
{
	return m_bundle.outDir();
}

/*****************************************************************************/
std::string AppBundlerLinux::getResourcePath() const
{
	return m_bundle.outDir();
}

}
