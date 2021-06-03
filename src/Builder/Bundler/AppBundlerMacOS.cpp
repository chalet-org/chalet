/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Builder/Bundler/AppBundlerMacOS.hpp"

#include "Builder/PlatformFile.hpp"
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
		"CFBundleDisplayName": "${name}",
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
AppBundlerMacOS::AppBundlerMacOS(BuildState& inState, const std::string& inBuildFile, BundleTarget& inBundle, const bool m_cleanOutput) :
	IAppBundler(inState, inBundle, m_cleanOutput),
	m_buildFile(inBuildFile)
{
}

/*****************************************************************************/
bool AppBundlerMacOS::removeOldFiles()
{
	return true;
}

/*****************************************************************************/
bool AppBundlerMacOS::bundleForPlatform()
{
	auto& macosBundle = m_bundle.macosBundle();

	m_bundlePath = getBundlePath();
	m_frameworkPath = fmt::format("{}/Frameworks", m_bundlePath);
	m_resourcePath = getResourcePath();
	m_executablePath = getExecutablePath();

	auto& bundleProjects = m_bundle.projects();

	std::string lastOutput;
	// TODO: Like with the linux bundler, this doesn't target a particular executable
	// This just gets the first
	for (auto& target : m_state.targets)
	{
		if (target->isProject())
		{
			auto& project = static_cast<const ProjectTarget&>(*target);
			if (!List::contains(bundleProjects, project.name()))
				continue;

			if (project.isStaticLibrary())
				continue;

			lastOutput = project.outputFile();

			if (!project.isExecutable())
				continue;

			// LOG("Main exec:", project.name());
			m_mainExecutable = project.outputFile();
			break;
		}
	}

	if (m_mainExecutable.empty())
	{
		m_mainExecutable = std::move(lastOutput);
		// return false;
	}

	if (m_mainExecutable.empty())
		return true;

	m_executableOutputPath = fmt::format("{}/{}", m_executablePath, m_mainExecutable);

	if (!changeRPathOfDependents(m_executableOutputPath))
		return false;

	// No app name = no bundle to make
	// treat it like linux/windows
	if (macosBundle.bundleName().empty())
	{
		if (!m_cleanOutput)
			Output::lineBreak();
		return true;
	}

	// TODO: Generalized version of this in AppBundler
	Output::print(Color::Blue, "   Creating the MacOS application bundle...");
	Output::lineBreak();

	Commands::makeDirectory(m_frameworkPath, m_cleanOutput);

	if (!createBundleIcon())
		return false;

	if (!createPListAndUpdateCommonKeys())
		return false;

	if (!setExecutablePaths())
		return false;

	if (!createDmgImage())
		return false;

	return true;
}

/*****************************************************************************/
std::string AppBundlerMacOS::getBundlePath() const
{
	const auto& outDir = m_bundle.outDir();
	const auto& bundleName = m_bundle.macosBundle().bundleName();
	if (!bundleName.empty())
	{
		return fmt::format("{}/{}.app/Contents", outDir, bundleName);
	}
	else
	{
		return outDir;
	}
}

/*****************************************************************************/
std::string AppBundlerMacOS::getExecutablePath() const
{
	const auto& bundleName = m_bundle.macosBundle().bundleName();
	if (!bundleName.empty())
	{
		return fmt::format("{}/MacOS", getBundlePath());
	}
	else
	{
		return m_bundle.outDir();
	}
}

/*****************************************************************************/
std::string AppBundlerMacOS::getResourcePath() const
{
	const auto& bundleName = m_bundle.macosBundle().bundleName();
	if (!bundleName.empty())
	{
		return fmt::format("{}/Resources", getBundlePath());
	}
	else
	{
		return m_bundle.outDir();
	}
}

/*****************************************************************************/
bool AppBundlerMacOS::changeRPathOfDependents(const std::string& inRPath) const
{
	auto& installNameTool = m_state.tools.installNameUtil();
	const auto& buildOutputDir = m_state.paths.buildOutputDir();

	StringList dylibs = m_bundle.macosBundle().dylibs();
	for (auto& target : m_state.targets)
	{
		if (target->isProject())
		{
			auto& project = static_cast<const ProjectTarget&>(*target);
			if (project.isSharedLibrary())
			{
				const auto targetPath = fmt::format("{}/{}", buildOutputDir, project.outputFile());
				if (!Commands::pathExists(targetPath))
					return false;

				// if (!Commands::copy(targetPath, m_frameworkPath, m_cleanOutput))
				// 	return false;

				dylibs.push_back(targetPath);
			}
		}
	}

	for (auto& dylib : dylibs)
	{
		// TODO: At the moment, this expects the full path
		const std::string filename = String::getPathFilename(dylib);

		const auto dylibBuild = fmt::format("{}/{}", m_executablePath, filename);
		if (!Commands::pathExists(dylibBuild))
		{
			auto d = Commands::which(dylib, m_cleanOutput);
			if (d.empty())
			{
				d = dylib;
				if (!Commands::pathExists(d))
					return false;

				dylib = filename;
			}

			if (!Commands::copy(d, m_executablePath, m_cleanOutput))
				return false;

			dylib = fmt::format("{}/{}", m_executablePath, dylib);
		}

		if (!Commands::subprocess({ installNameTool, "-change", dylib, fmt::format("@rpath/{}", filename), inRPath }, m_cleanOutput))
			return false;
	}

	// all should be copied by this point
	for (auto& dylib : dylibs)
	{
		const std::string filename = String::getPathFilename(dylib);
		const auto thisDylib = fmt::format("{}/{}", m_executablePath, filename);
		Commands::subprocess({ installNameTool, "-id", fmt::format("@rpath/{}", filename), thisDylib }, m_cleanOutput);

		for (auto& d : dylibs)
		{
			if (d == dylib)
				continue;

			const std::string fn = String::getPathFilename(d);
			const auto dylibBuild = fmt::format("{}/{}", m_executablePath, fn);
			Commands::subprocess({ installNameTool, "-change", d, fmt::format("@rpath/{}", fn), thisDylib }, m_cleanOutput);
		}
	}

	return true;
}

/*****************************************************************************/
bool AppBundlerMacOS::createBundleIcon()
{
	const auto& icon = m_bundle.macosBundle().icon();
	m_iconBaseName = String::getPathBaseName(icon);

	if (!icon.empty())
	{
		std::string outIcon;
		const auto& sips = m_state.tools.sips();
		bool sipsFound = !sips.empty();

		if (String::endsWith(".png", icon) && sipsFound)
		{
			outIcon = fmt::format("{}/{}.icns", m_resourcePath, m_iconBaseName);

			if (!Commands::subprocessNoOutput({ sips, "-s", "format", "icns", icon, "--out", outIcon }))
				return false;
		}
		else if (String::endsWith(".icns", icon))
		{
			outIcon = fmt::format("{}/{}.icns", m_resourcePath, m_iconBaseName);

			if (!Commands::copy(icon, m_resourcePath, m_cleanOutput))
				return false;
		}
		else
		{
			if (!icon.empty() && !sipsFound)
			{
				Diagnostic::warn(fmt::format("{}: Icon conversion from '{}' to icns requires the 'sips' command line tool.", m_buildFile, icon));
			}
		}
	}

	return true;
}

/*****************************************************************************/
bool AppBundlerMacOS::createPListAndUpdateCommonKeys() const
{
	auto& macosBundle = m_bundle.macosBundle();

	std::string tmpInfoPlist;
	const auto& infoPropertyList = macosBundle.infoPropertyList();
	const auto& infoPropertyListContent = macosBundle.infoPropertyListContent();
	if (infoPropertyList.empty())
	{
		if (infoPropertyListContent.empty())
			return true;

		const auto& outDir = m_bundle.outDir();
		tmpInfoPlist = fmt::format("{}/Info.plist.json", outDir);
		std::ofstream(tmpInfoPlist) << infoPropertyListContent << std::endl;
	}

	const auto& version = m_state.info.version();
	const auto& name = m_bundle.name();
	// const auto& bundleIdentifier = macosBundle.bundleIdentifier();
	const auto& bundleName = macosBundle.bundleName();

	const std::string outInfoPropertyList = fmt::format("{}/Info.plist", m_bundlePath);

	{
		const auto& plistInput = !tmpInfoPlist.empty() ? tmpInfoPlist : infoPropertyList;
		if (!m_state.tools.plistConvertToBinary(plistInput, outInfoPropertyList, m_cleanOutput))
			return false;

		if (!tmpInfoPlist.empty())
		{
			Commands::remove(tmpInfoPlist, m_cleanOutput);
			tmpInfoPlist.clear();
		}
	}

	if (!m_state.tools.plistReplaceProperty(outInfoPropertyList, "CFBundleName", bundleName, m_cleanOutput))
		return false;

	if (!m_iconBaseName.empty())
	{
		if (!m_state.tools.plistReplaceProperty(outInfoPropertyList, "CFBundleIconFile", m_iconBaseName, m_cleanOutput))
			return false;
	}

	if (!m_state.tools.plistReplaceProperty(outInfoPropertyList, "CFBundleDisplayName", name, m_cleanOutput))
		return false;

	// if (!m_state.tools.plistReplaceProperty(outInfoPropertyList, "CFBundleIdentifier", bundleIdentifier, m_cleanOutput))
	// 	return false;

	if (!m_state.tools.plistReplaceProperty(outInfoPropertyList, "CFBundleVersion", version, m_cleanOutput))
		return false;

	if (!m_state.tools.plistReplaceProperty(outInfoPropertyList, "CFBundleExecutable", m_mainExecutable, m_cleanOutput))
		return false;

	return true;
}

/*****************************************************************************/
bool AppBundlerMacOS::setExecutablePaths() const
{
	auto& installNameTool = m_state.tools.installNameUtil();

	// install_name_tool
	if (!Commands::subprocess({ installNameTool, "-add_rpath", "@executable_path/../MacOS", m_executableOutputPath }, m_cleanOutput))
		return false;

	if (!Commands::subprocess({ installNameTool, "-add_rpath", "@executable_path/../Frameworks", m_executableOutputPath }, m_cleanOutput))
		return false;

	if (!Commands::subprocess({ installNameTool, "-add_rpath", "@executable_path/../Resources", m_executableOutputPath }, m_cleanOutput))
		return false;

	for (auto& target : m_state.targets)
	{
		if (target->isProject())
		{
			auto& project = static_cast<const ProjectTarget&>(*target);
			for (auto& framework : project.macosFrameworks())
			{
				// Don't include System frameworks
				// TODO: maybe make an option for this? Not sure what scenarios this is needed
				if (Commands::pathExists(fmt::format("/System/Library/Frameworks/{}.framework", framework)))
					continue;

				for (auto& path : project.macosFrameworkPaths())
				{
					const std::string filename = fmt::format("{}{}.framework", path, framework);
					if (!Commands::pathExists(filename))
						continue;

					if (!Commands::copySkipExisting(filename, m_frameworkPath, m_cleanOutput))
						return false;

					const auto resolvedFramework = fmt::format("{}/{}.framework", m_frameworkPath, framework);

					if (!Commands::subprocess({ installNameTool, "-change", resolvedFramework, fmt::format("@rpath/{}", filename), m_executableOutputPath }, m_cleanOutput))
						return false;

					break;
				}
			}
		}
	}

	return true;
}

/*****************************************************************************/
bool AppBundlerMacOS::createDmgImage() const
{
	auto& macosBundle = m_bundle.macosBundle();
	const auto& bundleName = macosBundle.bundleName();
	if (!macosBundle.makeDmg())
		return true;

	const auto& outDir = m_bundle.outDir();

	auto& hdiutil = m_state.tools.hdiutil();
	auto& tiffutil = m_state.tools.tiffutil();
	const std::string volumePath = fmt::format("/Volumes/{}", bundleName);
	const std::string appPath = fmt::format("{}/{}.app", outDir, bundleName);

	Commands::subprocessNoOutput({ hdiutil, "detach", fmt::format("{}/", volumePath) }, m_cleanOutput);

	if (m_cleanOutput)
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

	if (!Commands::subprocessNoOutput({ hdiutil, "create", "-megabytes", fmt::format("{}", dmgSize), "-fs", "HFS+", "-volname", bundleName, tmpDmg }, m_cleanOutput))
		return false;

	if (!Commands::subprocessNoOutput({ hdiutil, "attach", tmpDmg }, m_cleanOutput))
		return false;

	if (!Commands::copy(appPath, volumePath, m_cleanOutput))
		return false;

	const std::string backgroundPath = fmt::format("{}/.background", volumePath);
	if (!Commands::makeDirectory(backgroundPath, m_cleanOutput))
		return false;

	const auto& background1x = macosBundle.dmgBackground1x();
	const auto& background2x = macosBundle.dmgBackground2x();

	if (!Commands::subprocessNoOutput({ tiffutil, "-cathidpicheck", background1x, background2x, "-out", fmt::format("{}/background.tiff", backgroundPath) }, m_cleanOutput))
		return false;

	if (!Commands::createDirectorySymbolicLink("/Applications", fmt::format("{}/Applications", volumePath), m_cleanOutput))
		return false;

	const auto applescriptText = PlatformFile::macosDmgApplescript(bundleName);

	if (!Commands::subprocess({ m_state.tools.osascript(), "-e", applescriptText }, m_cleanOutput))
		return false;
	if (!Commands::subprocess({ "rm", "-rf", fmt::format("{}/.fseventsd", volumePath) }, m_cleanOutput))
		return false;

	if (!Commands::subprocessNoOutput({ hdiutil, "detach", fmt::format("{}/", volumePath) }, m_cleanOutput))
		return false;

	const std::string outDmgPath = fmt::format("{}/{}.dmg", outDir, bundleName);
	if (!Commands::subprocessNoOutput({ hdiutil, "convert", tmpDmg, "-format", "UDZO", "-o", outDmgPath }, m_cleanOutput))
		return false;

	if (!Commands::removeRecursively(tmpDmg, m_cleanOutput))
		return false;

	if (m_cleanOutput)
	{
		Output::print(Color::Blue, fmt::format("   Done. See '{}'", outDmgPath));
	}

	Output::lineBreak();

	return true;
}
}
