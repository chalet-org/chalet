/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Bundler/AppBundlerMacOS.hpp"

#include "FileTemplates/PlatformFileTemplates.hpp"

#include "State/AncillaryTools.hpp"
#include "State/Distribution/BundleTarget.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Environment.hpp"
#include "Terminal/Output.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"
#include "Utility/Timer.hpp"

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
AppBundlerMacOS::AppBundlerMacOS(BuildState& inState, const BundleTarget& inBundle, BinaryDependencyMap& inDependencyMap, const std::string& inBuildFile) :
	IAppBundler(inState, inBundle, inDependencyMap),
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
	m_frameworkPath = getFrameworksPath();
	m_resourcePath = getResourcePath();
	m_executablePath = getExecutablePath();

	if (!getMainExecutable())
		return true; // No executable. we don't care

	m_executableOutputPath = fmt::format("{}/{}", m_executablePath, m_mainExecutable);

	auto& installNameTool = m_state.ancillaryTools.installNameTool();
	if (m_bundle.updateRPaths())
	{
		if (!changeRPathOfDependents(installNameTool, m_dependencyMap, m_executablePath))
			return false;
	}

	if (!Commands::subprocess({ installNameTool, "-add_rpath", "@executable_path/.", m_executableOutputPath }))
		return false;

	// No app name = no bundle to make
	// treat it like linux/windows
	if (macosBundle.bundleName().empty())
	{
		Output::lineBreak();

		return signAppBundle();
	}

	// TODO: Generalized version of this in AppBundler
	Output::lineBreak();

	Commands::makeDirectory(m_frameworkPath);

	if (!createBundleIcon())
		return false;

	if (!createPListAndUpdateCommonKeys())
		return false;

	if (!setExecutablePaths())
		return false;

	if (!signAppBundle())
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
std::string AppBundlerMacOS::getFrameworksPath() const
{
	const auto& bundleName = m_bundle.macosBundle().bundleName();
	if (!bundleName.empty())
	{
		return fmt::format("{}/Frameworks", getBundlePath());
	}
	else
	{
		return m_bundle.outDir();
	}
}

/*****************************************************************************/
bool AppBundlerMacOS::changeRPathOfDependents(const std::string& inInstallNameTool, BinaryDependencyMap& inDependencyMap, const std::string& inExecutablePath)
{
	for (auto& [file, dependencies] : inDependencyMap)
	{
		auto filename = String::getPathFilename(file);
		const auto outputFile = fmt::format("{}/{}", inExecutablePath, filename);

		if (!changeRPathOfDependents(inInstallNameTool, filename, dependencies, outputFile))
			return false;
	}

	return true;
}

/*****************************************************************************/
bool AppBundlerMacOS::changeRPathOfDependents(const std::string& inInstallNameTool, const std::string& inFile, const StringList& inDependencies, const std::string& inOutputFile)
{
	if (inDependencies.size() > 0)
	{
		if (!Commands::subprocess({ inInstallNameTool, "-id", fmt::format("@rpath/{}", inFile), inOutputFile }))
		{
			Diagnostic::error("install_name_tool error");
			return false;
		}
	}

	for (auto& dep : inDependencies)
	{
		auto depFile = String::getPathFilename(dep);
		if (!Commands::subprocess({ inInstallNameTool, "-change", dep, fmt::format("@rpath/{}", depFile), inOutputFile }))
		{
			Diagnostic::error("install_name_tool error");
			return false;
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
		std::string outIcon = fmt::format("{}/{}.icns", m_resourcePath, m_iconBaseName);
		const auto& sips = m_state.ancillaryTools.sips();
		bool sipsFound = !sips.empty();

		if (String::endsWith(".png", icon) && sipsFound)
		{
			if (!Commands::subprocessNoOutput({ sips, "-s", "format", "icns", icon, "--out", outIcon }))
				return false;
		}
		else if (String::endsWith(".icns", icon))
		{
			if (!Commands::copy(icon, m_resourcePath))
				return false;
		}
		else
		{
			if (!icon.empty() && !sipsFound)
			{
				Diagnostic::warn("{}: Icon conversion from '{}' to icns requires the 'sips' command line tool.", m_buildFile, icon);
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
		{
			Diagnostic::error("No info plist or plist content");
			return true;
		}

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
		if (!m_state.ancillaryTools.plistConvertToBinary(plistInput, outInfoPropertyList))
			return false;

		if (!tmpInfoPlist.empty())
		{
			Commands::remove(tmpInfoPlist);
			tmpInfoPlist.clear();
		}
	}

	if (!m_state.ancillaryTools.plistReplaceProperty(outInfoPropertyList, "CFBundleName", bundleName))
		return false;

	if (!m_iconBaseName.empty())
	{
		if (!m_state.ancillaryTools.plistReplaceProperty(outInfoPropertyList, "CFBundleIconFile", m_iconBaseName))
			return false;
	}

	if (!m_state.ancillaryTools.plistReplaceProperty(outInfoPropertyList, "CFBundleDisplayName", name))
		return false;

	// if (!m_state.ancillaryTools.plistReplaceProperty(outInfoPropertyList, "CFBundleIdentifier", bundleIdentifier))
	// 	return false;

	if (!m_state.ancillaryTools.plistReplaceProperty(outInfoPropertyList, "CFBundleVersion", version))
		return false;

	if (!m_state.ancillaryTools.plistReplaceProperty(outInfoPropertyList, "CFBundleExecutable", m_mainExecutable))
		return false;

	return true;
}

/*****************************************************************************/
bool AppBundlerMacOS::setExecutablePaths() const
{
	auto& installNameTool = m_state.ancillaryTools.installNameTool();

	for (auto p : m_state.environment.path())
	{
		String::replaceAll(p, m_state.paths.buildOutputDir() + '/', "");
		Commands::subprocessNoOutput({ installNameTool, "-delete_rpath", fmt::format("@executable_path/{}", p), m_executableOutputPath });
	}

	// install_name_tool
	if (!Commands::subprocess({ installNameTool, "-add_rpath", "@executable_path/../MacOS", m_executableOutputPath }))
		return false;

	if (!Commands::subprocess({ installNameTool, "-add_rpath", "@executable_path/../Frameworks", m_executableOutputPath }))
		return false;

	if (!Commands::subprocess({ installNameTool, "-add_rpath", "@executable_path/../Resources", m_executableOutputPath }))
		return false;

	uint i = 0;
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

					if (!Commands::copySkipExisting(filename, m_frameworkPath))
						return false;

					const auto resolvedFramework = fmt::format("{}/{}.framework", m_frameworkPath, framework);

					if (!Commands::subprocess({ installNameTool, "-change", resolvedFramework, fmt::format("@rpath/{}", filename), m_executableOutputPath }))
						return false;

					++i;

					break;
				}
			}
		}
	}
	if (i > 0)
		Output::lineBreak();

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

	auto& hdiutil = m_state.ancillaryTools.hdiutil();
	auto& tiffutil = m_state.ancillaryTools.tiffutil();
	const std::string volumePath = fmt::format("/Volumes/{}", bundleName);
	const std::string appPath = fmt::format("{}/{}.app", outDir, bundleName);

	Commands::subprocessNoOutput({ hdiutil, "detach", fmt::format("{}/", volumePath) });

	Timer timer;

	const auto& universalBinaryArches = macosBundle.universalBinaryArches();
	if (universalBinaryArches.size() < 2)
	{
		Output::lineBreak();
	}
	Diagnostic::info("Creating the distribution disk image", false);

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

	if (!Commands::subprocessNoOutput({ hdiutil, "create", "-megabytes", fmt::format("{}", dmgSize), "-fs", "HFS+", "-volname", bundleName, tmpDmg }))
		return false;

	if (!Commands::subprocessNoOutput({ hdiutil, "attach", tmpDmg }))
		return false;

	if (!Commands::copySilent(appPath, volumePath))
		return false;

	const std::string backgroundPath = fmt::format("{}/.background", volumePath);
	if (!Commands::makeDirectory(backgroundPath))
		return false;

	const auto& background1x = macosBundle.dmgBackground1x();
	const auto& background2x = macosBundle.dmgBackground2x();

	if (!Commands::subprocessNoOutput({ tiffutil, "-cathidpicheck", background1x, background2x, "-out", fmt::format("{}/background.tiff", backgroundPath) }))
		return false;

	if (!Commands::createDirectorySymbolicLink("/Applications", fmt::format("{}/Applications", volumePath)))
		return false;

	const auto applescriptText = PlatformFileTemplates::macosDmgApplescript(bundleName);

	if (!Commands::subprocess({ m_state.ancillaryTools.osascript(), "-e", applescriptText }))
		return false;
	if (!Commands::subprocess({ "rm", "-rf", fmt::format("{}/.fseventsd", volumePath) }))
		return false;

	if (!Commands::subprocessNoOutput({ hdiutil, "detach", fmt::format("{}/", volumePath) }))
		return false;

	const std::string outDmgPath = fmt::format("{}/{}.dmg", outDir, bundleName);
	if (!Commands::subprocessNoOutput({ hdiutil, "convert", tmpDmg, "-format", "UDZO", "-o", outDmgPath }))
		return false;

	if (!Commands::removeRecursively(tmpDmg))
		return false;

	Diagnostic::printDone(timer.asString());

	return signDmgImage(outDmgPath);
}

/*****************************************************************************/
bool AppBundlerMacOS::signAppBundle() const
{
	if (m_state.ancillaryTools.macosSigningIdentity().empty())
	{
		Diagnostic::warn("bundle '{}' was not signed - macosSigningIdentity is not set, or was empty.", m_bundle.name());
		return true;
	}

	Timer timer;
	Diagnostic::info("Signing the MacOS application bundle", false);

	// TODO: Entitlements
	bool isBundle = String::endsWith(".app/Contents", m_bundlePath);

	try
	{
		for (const fs::directory_entry& entry : fs::recursive_directory_iterator(m_executablePath))
		{
			if (entry.is_regular_file())
			{
				auto path = entry.path().string();
				if (!m_state.ancillaryTools.macosCodeSignFile(path))
				{
					Diagnostic::error("Failed to sign: {}", path);
					return false;
				}
			}
		}

		if (isBundle)
		{
			for (const fs::directory_entry& entry : fs::recursive_directory_iterator(m_frameworkPath))
			{
				if (entry.is_regular_file())
				{
					auto path = entry.path().string();
					if (!m_state.ancillaryTools.macosCodeSignFile(path))
					{
						Diagnostic::error("Failed to sign: {}", path);
						return false;
					}
				}
			}

			for (const fs::directory_entry& entry : fs::recursive_directory_iterator(m_resourcePath))
			{
				if (entry.is_regular_file())
				{
					auto path = entry.path().string();
					if (!m_state.ancillaryTools.macosCodeSignFile(path))
					{
						Diagnostic::error("Failed to sign: {}", path);
						return false;
					}
				}
			}

			auto appPath = m_bundlePath.substr(0, m_bundlePath.size() - 9);
			if (!m_state.ancillaryTools.macosCodeSignFile(appPath))
			{
				Diagnostic::error("Failed to sign: {}", appPath);
				return false;
			}
		}

		Diagnostic::printDone(timer.asString());

		return true;
	}
	catch (const fs::filesystem_error& err)
	{
		Diagnostic::error(err.what());
		return false;
	}
}

/*****************************************************************************/
bool AppBundlerMacOS::signDmgImage(const std::string& inPath) const
{
	if (m_state.ancillaryTools.macosSigningIdentity().empty())
	{
		Diagnostic::warn("dmg '{}' was not signed - macosSigningIdentity is not set, or was empty.", inPath);
		return true;
	}

	Timer timer;
	Diagnostic::info("Signing the disk image", false);

	if (!m_state.ancillaryTools.macosCodeSignDiskImage(inPath))
	{
		Diagnostic::error("Failed to sign: {}", inPath);
		return false;
	}

	Diagnostic::printDone(timer.asString());

	return true;
}
}
