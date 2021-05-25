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
AppBundlerMacOS::AppBundlerMacOS(BuildState& inState, const std::string& inBuildFile) :
	m_state(inState),
	m_buildFile(inBuildFile)
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

	const auto& buildOutputDir = m_state.paths.buildOutputDir();
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
		{
			Diagnostic::warn(fmt::format("{}: Icon conversion from '{}' to icns requires the 'sips' command line tool.", m_buildFile, icon));
		}
	}

	const std::string outInfoPropertyList = fmt::format("{}/Info.plist", bundlePath);

	// TODO: Like with the linux bundler, this doesn't target a particular executable
	// This just gets the first
	std::string mainExecutable;
	for (auto& project : m_state.projects)
	{
		if (project->hasScripts())
			continue;

		if (!project->isExecutable())
			continue;

		if (!List::contains(bundleProjects, project->name()))
			continue;

		// LOG("Main exec:", project->name());
		mainExecutable = project->outputFile();
		break;
	}

	if (mainExecutable.empty())
	{
		Diagnostic::error("No projects defined for bundle");
		return false;
	}

	// PList
	if (!m_state.tools.plistConvertToBinary(infoPropertyList, outInfoPropertyList, inCleanOutput))
		return false;
	if (!m_state.tools.plistReplaceProperty(outInfoPropertyList, "CFBundleName", bundleName, inCleanOutput))
		return false;
	if (!m_state.tools.plistReplaceProperty(outInfoPropertyList, "CFBundleIconFile", iconBaseName, inCleanOutput))
		return false;
	if (!m_state.tools.plistReplaceProperty(outInfoPropertyList, "CFBundleDisplayName", appName, inCleanOutput))
		return false;
	if (!m_state.tools.plistReplaceProperty(outInfoPropertyList, "CFBundleIdentifier", bundleIdentifier, inCleanOutput))
		return false;
	if (!m_state.tools.plistReplaceProperty(outInfoPropertyList, "CFBundleVersion", version, inCleanOutput))
		return false;
	if (!m_state.tools.plistReplaceProperty(outInfoPropertyList, "CFBundleExecutable", mainExecutable, inCleanOutput))
		return false;

	// install_name_tool
	auto& installNameTool = m_state.tools.installNameUtil();
	const auto executableOutputPath = fmt::format("{}/{}", executablePath, mainExecutable);
	if (!Commands::subprocess({ installNameTool, "-add_rpath", "@executable_path/../MacOS", executableOutputPath }, inCleanOutput))
		return false;

	if (!Commands::subprocess({ installNameTool, "-add_rpath", "@executable_path/../Frameworks", executableOutputPath }, inCleanOutput))
		return false;

	if (!Commands::subprocess({ installNameTool, "-add_rpath", "@executable_path/../Resources", executableOutputPath }, inCleanOutput))
		return false;

	StringList dylibs = macosBundle.dylibs();
	for (auto& project : m_state.projects)
	{
		if (project->hasScripts())
			continue;

		if (!project->cmake())
		{
			if (project->isSharedLibrary())
			{
				const auto target = fmt::format("{}/{}", buildOutputDir, project->outputFile());
				if (!Commands::pathExists(target))
					return false;

				// if (!Commands::copy(target, frameworkPath, inCleanOutput))
				// 	return false;

				dylibs.push_back(target);
			}
		}
	}

	for (auto& dylib : dylibs)
	{
		// TODO: At the moment, this expects the full path
		const std::string filename = String::getPathFilename(dylib);

		const auto dylibBuild = fmt::format("{}/{}", executablePath, filename);
		if (!Commands::pathExists(dylibBuild))
		{
			auto d = Commands::which(dylib, inCleanOutput);
			if (d.empty())
			{
				d = dylib;
				if (!Commands::pathExists(d))
					return false;

				dylib = filename;
			}

			if (!Commands::copy(d, executablePath, inCleanOutput))
				return false;

			dylib = fmt::format("{}/{}", executablePath, dylib);
		}

		if (!Commands::subprocess({ installNameTool, "-change", dylib, fmt::format("@rpath/{}", filename), executableOutputPath }, inCleanOutput))
			return false;
	}

	// all should be copied by this point
	for (auto& dylib : dylibs)
	{
		const std::string filename = String::getPathFilename(dylib);
		const auto thisDylib = fmt::format("{}/{}", executablePath, filename);
		Commands::subprocess({ installNameTool, "-id", fmt::format("@rpath/{}", filename), thisDylib }, inCleanOutput);

		for (auto& d : dylibs)
		{
			if (d == dylib)
				continue;

			const std::string fn = String::getPathFilename(d);
			const auto dylibBuild = fmt::format("{}/{}", executablePath, fn);
			Commands::subprocess({ installNameTool, "-change", d, fmt::format("@rpath/{}", fn), thisDylib }, inCleanOutput);
		}
	}

	for (auto& project : m_state.projects)
	{
		if (project->hasScripts())
			continue;

		for (auto& framework : project->macosFrameworks())
		{
			// Don't include System frameworks
			// TODO: maybe make an option for this? Not sure what scenarios this is needed
			if (Commands::pathExists(fmt::format("/System/Library/Frameworks/{}.framework", framework)))
				continue;

			for (auto& path : project->macosFrameworkPaths())
			{
				const std::string filename = fmt::format("{}{}.framework", path, framework);
				if (!Commands::pathExists(filename))
					continue;

				if (!Commands::copySkipExisting(filename, frameworkPath, inCleanOutput))
					return false;

				const auto resolvedFramework = fmt::format("{}/{}.framework", frameworkPath, framework);

				if (!Commands::subprocess({ installNameTool, "-change", resolvedFramework, fmt::format("@rpath/{}", filename), executableOutputPath }, inCleanOutput))
					return false;

				break;
			}
		}
	}

	if (macosBundle.makeDmg())
	{
		auto& hdiutil = m_state.tools.hdiutil();
		auto& tiffutil = m_state.tools.tiffutil();
		const std::string volumePath = fmt::format("/Volumes/{}", bundleName);
		const std::string appPath = fmt::format("{}/{}.app", outDir, bundleName);

		Commands::subprocessNoOutput({ hdiutil, "detach", fmt::format("{}/", volumePath) }, inCleanOutput);

		if (inCleanOutput)
		{
			Output::print(Color::Blue, "   Creating the disk image for the application...");
			Output::lineBreak();
		}

		const std::string tmpDmg = fmt::format("{}/.tmp.dmg", outDir);

		std::uintmax_t appSize = Commands::getPathSize(appPath);
		std::uintmax_t mb = 1000000;
		std::uintmax_t dmgSize = appSize > mb ? appSize / mb : 10;
		// if (dmgSize > 10)
		{
			std::uintmax_t temp = 16;
			while (temp < dmgSize)
			{
				temp += temp;
			}
			dmgSize = temp;
		}

		if (!Commands::subprocessNoOutput({ hdiutil, "create", "-megabytes", fmt::format("{}", dmgSize), "-fs", "HFS+", "-volname", bundleName, tmpDmg }, inCleanOutput))
			return false;

		if (!Commands::subprocessNoOutput({ hdiutil, "attach", tmpDmg }, inCleanOutput))
			return false;

		if (!Commands::copy(appPath, volumePath, inCleanOutput))
			return false;

		const std::string backgroundPath = fmt::format("{}/.background", volumePath);
		if (!Commands::makeDirectory(backgroundPath, inCleanOutput))
			return false;

		const auto& background1x = macosBundle.dmgBackground1x();
		const auto& background2x = macosBundle.dmgBackground2x();

		if (!Commands::subprocessNoOutput({ tiffutil, "-cathidpicheck", background1x, background2x, "-out", fmt::format("{}/background.tiff", backgroundPath) }, inCleanOutput))
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

		if (!Commands::subprocessNoOutput({ hdiutil, "detach", fmt::format("{}/", volumePath) }, inCleanOutput))
			return false;

		const std::string outDmgPath = fmt::format("{}/{}.dmg", outDir, bundleName);
		if (!Commands::subprocessNoOutput({ hdiutil, "convert", tmpDmg, "-format", "UDZO", "-o", outDmgPath }, inCleanOutput))
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
