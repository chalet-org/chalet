/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Builder/Bundler/AppBundlerMacOS.hpp"

#include "Libraries/Format.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Environment.hpp"
#include "Terminal/Output.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*
	{
		"CFBundleName": "${bundleName}",
		"CFBundleDisplayName": "${appName}",
		"CFBundleIdentifier": "${bundleIdentifier}",
		"CFBundleVersion": "${version}",
		"CFBundleDevelopmentRegion": "en",
		"CFBundleInfoDictionaryVersion": "6.0",
		"CFBundlePackageType": "APPL",
		"CFBundleSignature": "????",
		"CFBundleExecutable": "${mainProject}",
		"CFBundleIconFile": "${icon}",
		"NSHighResolutionCapable": true
	}
*/

/*****************************************************************************/
AppBundlerMacOS::AppBundlerMacOS(BuildState& inState) :
	m_state(inState)
{
	// TODO: Generalized version of this in AppBundler
	Output::print(Color::Blue, "   Creating the MacOS application bundle...");
	Output::lineBreak();
}

/*****************************************************************************/
bool AppBundlerMacOS::removeOldFiles(const bool inCleanOutput)
{
	UNUSED(inCleanOutput);

	return true;
}

/*****************************************************************************/
bool AppBundlerMacOS::bundleForPlatform(const bool inCleanOutput)
{
	auto& bundle = m_state.bundle;
	auto& macosBundle = bundle.macosBundle();
	auto& bundleProjects = bundle.projects();

	const auto& outDir = bundle.outDir();

	const auto& version = m_state.info.version();
	const auto& icon = macosBundle.icon();
	const auto& appName = bundle.appName();
	const auto& bundleIdentifier = macosBundle.bundleIdentifier();
	const auto& bundleName = macosBundle.bundleName();
	const auto& infoPropertyList = macosBundle.infoPropertyList();

	const auto bundlePath = getBundlePath();
	const auto frameworkPath = fmt::format("{}/Frameworks", bundlePath);
	const auto resourcePath = getResourcePath();
	const auto executablePath = getExecutablePath();

	Commands::makeDirectory(frameworkPath, inCleanOutput);

	const auto& sips = m_state.tools.sips();
	bool sipsFound = !sips.empty();

	std::string outIcon;
	const std::string iconBaseName = String::getPathBaseName(icon);

	if (String::endsWith(".png", icon) && sipsFound)
	{
		outIcon = fmt::format("{}/{}.icns", resourcePath, iconBaseName);

		if (!Commands::subprocessNoOutput({ sips, "-s", "format", "icns", icon, "--out", outIcon }))
			return false;
	}
	else if (String::endsWith(".icns", icon))
	{
		outIcon = fmt::format("{}/{}.icns", resourcePath, iconBaseName);

		if (!Commands::copy(icon, resourcePath, inCleanOutput))
			return false;
	}
	else
	{
		if (!icon.empty() && !sipsFound)
			Diagnostic::warn(fmt::format("{}: Icon conversion from '{}' to icns requires the 'sips' command line tool.", CommandLineInputs::file(), icon));
	}

	const std::string outInfoPropertyList = fmt::format("{}/Info.plist", bundlePath);

	// TODO: Like with the linux bundler, this doesn't target a particular executable
	// This just gets the first
	std::string mainExecutable;
	for (auto& project : m_state.projects)
	{
		if (!project->includeInBuild())
			continue;

		if (!List::contains(bundleProjects, project->name()))
			continue;

		if (!project->isExecutable())
			continue;

		mainExecutable = project->outputFile();
		break;
	}

	{
		bool resPlist = true;
		resPlist &= m_state.tools.plistConvertToBinary(infoPropertyList, outInfoPropertyList, inCleanOutput);
		resPlist &= m_state.tools.plistReplaceProperty(outInfoPropertyList, "CFBundleName", bundleName, inCleanOutput);
		resPlist &= m_state.tools.plistReplaceProperty(outInfoPropertyList, "CFBundleIconFile", iconBaseName, inCleanOutput);
		resPlist &= m_state.tools.plistReplaceProperty(outInfoPropertyList, "CFBundleDisplayName", appName, inCleanOutput);
		resPlist &= m_state.tools.plistReplaceProperty(outInfoPropertyList, "CFBundleIdentifier", bundleIdentifier, inCleanOutput);
		resPlist &= m_state.tools.plistReplaceProperty(outInfoPropertyList, "CFBundleVersion", version, inCleanOutput);
		resPlist &= m_state.tools.plistReplaceProperty(outInfoPropertyList, "CFBundleExecutable", mainExecutable, inCleanOutput);

		if (!resPlist)
			return false;
	}

	auto& installNameTool = m_state.tools.installNameUtil();
	if (!Commands::subprocess({ installNameTool, "-add_rpath", "@executable_path/../Frameworks", fmt::format("{}/{}", executablePath, mainExecutable) }, inCleanOutput))
		return false;

	if (!Commands::subprocess({ installNameTool, "-add_rpath", "@loader_path/..", fmt::format("{}/{}", executablePath, mainExecutable) }, inCleanOutput))
		return false;

	{
		bool resCopies = true;
		for (auto& dep : macosBundle.dylibs())
		{
			// TODO: At the moment, this expects the full path
			const std::string filename = String::getPathFilename(dep);

			bool result = true;
			result = Commands::copy(dep, executablePath, inCleanOutput);
			if (result)
			{
				result &= Commands::subprocess({ installNameTool, "-change", fmt::format("@rpath/{}", filename), fmt::format("@rpath/MacOS/{}", filename), executablePath }, inCleanOutput);
				result &= Commands::subprocess({ installNameTool, "-change", filename, fmt::format("@rpath/MacOS/{}", filename), executablePath }, inCleanOutput);
				// result &= Commands::subprocess({ installNameTool, "-change", dep, fmt::format("@rpath/MacOS/{}", dep), executablePath }, inCleanOutput);
			}
			resCopies &= result;
		}

		for (auto& project : m_state.projects)
		{
			if (!project->includeInBuild())
				continue;

			for (auto& dep : project->macosFrameworks())
			{
				for (auto& path : project->macosFrameworkPaths())
				{
					const std::string filename = fmt::format("{}/{}.framework", path, dep);
					if (!Commands::pathExists(filename))
						continue;

					resCopies &= Commands::copy(filename, frameworkPath, inCleanOutput);
				}
			}
		}

		if (!resCopies)
			return false;
	}

	if (macosBundle.makeDmg())
	{
		auto& hdiUtil = m_state.tools.hdiUtil();
		auto& tiffUtil = m_state.tools.tiffUtil();
		const std::string volumePath = fmt::format("/Volumes/{}", bundleName);
		if (!Commands::subprocessNoOutput({ hdiUtil, "detach", fmt::format("{}/", volumePath) }, inCleanOutput))
			return false;

		if (inCleanOutput)
		{
			Output::print(Color::Blue, "   Creating the disk image for the application...");
		}

		const std::string tmpDmg = fmt::format("{}/.tmp.dmg", outDir);
		if (!Commands::subprocessNoOutput({ hdiUtil, "create", "-megabytes", "54", "-fs", "HFS+", "-volname", bundleName, tmpDmg }, inCleanOutput))
			return false;

		if (!Commands::subprocessNoOutput({ hdiUtil, "attach", tmpDmg }, inCleanOutput))
			return false;

		const std::string appPath = fmt::format("{}/{}.app", outDir, bundleName);
		if (!Commands::copy(appPath, volumePath, inCleanOutput))
			return false;

		const std::string backgroundPath = fmt::format("{}/.background", volumePath);
		if (!Commands::makeDirectory(backgroundPath, inCleanOutput))
			return false;

		const auto& background1x = macosBundle.dmgBackground1x();
		const auto& background2x = macosBundle.dmgBackground2x();

		if (!Commands::subprocessNoOutput({ tiffUtil, "-cathidpicheck", background1x, background2x, "-out", fmt::format("{}/background.tiff", backgroundPath) }, inCleanOutput))
			return false;

		if (!Commands::createDirectorySymbolicLink("/Applications", fmt::format("{}/Applications", volumePath), inCleanOutput))
			return false;

		// const std::string applescriptPath = "env/osx/dmg.applescript";
		// Environment::set("CHALET_MACOS_BUNDLE_NAME", bundleName);

		const std::string applescriptText = fmt::format(R"applescript(set bundleName to "{bundleName}"
set appNameExt to "{bundleName}.app"
tell application "Finder"
 tell disk bundleName
  open
  set current view of container window to icon view
  set toolbar visible of container window to false
  set statusbar visible of container window to false
  set the bounds of container window to {{0, 0, 512, 342}}
  set viewOptions to the icon view options of container window
  set arrangement of viewOptions to not arranged
  set icon size of viewOptions to 80
  set background picture of viewOptions to file ".background:background.tiff"
  set position of item appNameExt of container window to {{120, 188}}
  set position of item "Applications" of container window to {{392, 188}}
  set position of item ".background" of container window to {{120, 388}}
  close
  update without registering applications
  delay 2
 end tell
end tell)applescript",
			FMT_ARG(bundleName));

		if (!Commands::subprocess({ m_state.tools.osascript(), "-e", applescriptText }, inCleanOutput))
			return false;
		if (!Commands::subprocess({ "rm", "-rf", fmt::format("{}/.fseventsd", volumePath) }, inCleanOutput))
			return false;

		if (!Commands::subprocessNoOutput({ hdiUtil, "detach", fmt::format("{}/", volumePath) }, inCleanOutput))
			return false;

		const std::string outDmgPath = fmt::format("{}/{}.dmg", outDir, bundleName);
		if (!Commands::subprocessNoOutput({ hdiUtil, "convert", tmpDmg, "-format", "UDZO", "-o", outDmgPath }, inCleanOutput))
			return false;

		if (!Commands::removeRecursively(tmpDmg, inCleanOutput))
			return false;

		if (inCleanOutput)
		{
			Output::print(Color::Blue, fmt::format("   Done. See '{}'", outDmgPath));
		}

		Output::lineBreak();
	}

	return true;
}

/*****************************************************************************/
std::string AppBundlerMacOS::getBundlePath() const
{
	const auto& outDir = m_state.bundle.outDir();
	const auto& bundleName = m_state.bundle.macosBundle().bundleName();

	return fmt::format("{}/{}.app/Contents", outDir, bundleName);
}

std::string AppBundlerMacOS::getExecutablePath() const
{
	return fmt::format("{}/MacOS", getBundlePath());
}

std::string AppBundlerMacOS::getResourcePath() const
{
	return fmt::format("{}/Resources", getBundlePath());
}

}
