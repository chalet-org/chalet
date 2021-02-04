/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Builder/Bundler/AppBundlerMacOS.hpp"

#include "Libraries/Format.hpp"
#include "Terminal/Commands.hpp"
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

		const std::string cmd = fmt::format("\"{sips}\" -s format icns '{icon}' --out '{outIcon}' &> /dev/null", FMT_ARG(sips), FMT_ARG(icon), FMT_ARG(outIcon));
		if (!Commands::shell(cmd))
			return false;

		if (inCleanOutput)
			Output::lineBreak();
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
	if (!Commands::shell(fmt::format("{} -add_rpath @executable_path/../Frameworks '{}/{}'", installNameTool, executablePath, mainExecutable), inCleanOutput))
		return false;

	if (!Commands::shell(fmt::format("{} -add_rpath @loader_path/.. '{}/{}'", installNameTool, executablePath, mainExecutable), inCleanOutput))
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
				result &= Commands::shell(fmt::format("{} -change @rpath/{0} @rpath/MacOS/{0} '{1}'", installNameTool, filename, executablePath), inCleanOutput);
				result &= Commands::shell(fmt::format("{} -change {0} @rpath/MacOS/{0} '{1}'", installNameTool, filename, executablePath), inCleanOutput);
				// result &= Commands::shell(fmt::format("{} -change {0} @rpath/MacOS/{0} '{1}'", m_state.tools.installNameUtil(), dep, executablePath), inCleanOutput);
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
		bool dmgResult = true;
		const std::string volumePath = fmt::format("/Volumes/{}", bundleName);
		dmgResult &= Commands::shell(fmt::format("{} detach '{}/' &> /dev/null", hdiUtil, volumePath), inCleanOutput);

		if (inCleanOutput)
		{
			Output::print(Color::Blue, "   Creating the disk image for the application...");
		}

		const std::string tmpDmg = fmt::format("{}/.tmp.dmg", outDir);
		dmgResult &= Commands::shell(fmt::format("{} create -megabytes 54 -fs HFS+ -volname '{}' '{}' &> /dev/null", hdiUtil, bundleName, tmpDmg), inCleanOutput);
		dmgResult &= Commands::shell(fmt::format("{} attach '{}' &> /dev/null", hdiUtil, tmpDmg), inCleanOutput);

		const std::string appPath = fmt::format("{}/{}.app", outDir, bundleName);
		dmgResult &= Commands::shell(fmt::format("cp -r '{}' '{}'", appPath, volumePath), inCleanOutput);

		const std::string backgroundPath = fmt::format("{}/.background", volumePath);
		dmgResult &= Commands::makeDirectory(backgroundPath, inCleanOutput);

		const auto& background1x = macosBundle.dmgBackground1x();
		const auto& background2x = macosBundle.dmgBackground2x();

		dmgResult &= Commands::shell(fmt::format("{} -cathidpicheck '{}' '{}' -out '{}/background.tiff' &> /dev/null", tiffUtil, background1x, background2x, backgroundPath), inCleanOutput);

		dmgResult &= Commands::createDirectorySymbolicLink("/Applications", fmt::format("{}/Applications", volumePath), inCleanOutput);

		// TODO: cache env/osx/dmg.applescript, pass more variable to it (size, etc)
		const std::string applescriptPath = "env/osx/dmg.applescript";
		dmgResult &= Commands::shell(fmt::format("appName='{}' osascript '{}'", bundleName, applescriptPath), inCleanOutput);
		dmgResult &= Commands::shellRemove(fmt::format("{}/.fseventsd", volumePath), inCleanOutput);

		dmgResult &= Commands::shell(fmt::format("{} detach '{}/' &> /dev/null", hdiUtil, volumePath), inCleanOutput);

		const std::string outDmgPath = fmt::format("{}/{}.dmg", outDir, bundleName);
		dmgResult &= Commands::shell(fmt::format("{} convert '{}' -format UDZO -o '{}' &> /dev/null", hdiUtil, tmpDmg, outDmgPath), inCleanOutput);

		dmgResult &= Commands::remove(tmpDmg, inCleanOutput);

		if (inCleanOutput)
		{
			Output::print(Color::Blue, fmt::format("   Done. See '{}'", outDmgPath));
		}

		Output::lineBreak();

		return dmgResult;
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
