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
		"CFBundleExecutable": "${mainExecutable}",
		"CFBundleIconFile": "${icon}",
		"NSHighResolutionCapable": true
	}
*/

/*****************************************************************************/
AppBundlerMacOS::AppBundlerMacOS(BuildState& inState, const BundleTarget& inBundle, BinaryDependencyMap& inDependencyMap, const std::string& inInputFile) :
	IAppBundler(inState, inBundle, inDependencyMap),
	m_inputFile(inInputFile)
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

	if (!Commands::pathExists(m_frameworkPath))
		Commands::makeDirectory(m_frameworkPath);

	auto& installNameTool = m_state.tools.installNameTool();
	if (m_bundle.updateRPaths())
	{
		if (!changeRPathOfDependents(installNameTool, m_dependencyMap, m_executablePath))
			return false;
	}

	if (!Commands::subprocess({ installNameTool, "-add_rpath", "@executable_path/.", m_executableOutputPath }))
		return false;

	// treat it like linux/windows
	if (macosBundle.bundleType() == MacOSBundleType::None)
	{
		Output::lineBreak();

		return signAppBundle();
	}
	else
	{
		// TODO: Generalized version of this in AppBundler
		Output::lineBreak();

		if (!createBundleIcon())
			return false;

		if (!createPListAndReplaceVariables())
			return false;

		if (!setExecutablePaths())
			return false;

		if (!signAppBundle())
			return false;

		if (!createDmgImage())
			return false;

		return true;
	}
}

/*****************************************************************************/
std::string AppBundlerMacOS::getBundlePath() const
{
	const auto& subDirectory = m_bundle.subDirectory();
	if (m_bundle.macosBundle().isAppBundle())
	{
		return fmt::format("{}/{}.app/Contents", subDirectory, m_bundle.name());
	}
	else
	{
		return subDirectory;
	}
}

/*****************************************************************************/
std::string AppBundlerMacOS::getExecutablePath() const
{
	if (m_bundle.macosBundle().isAppBundle())
	{
		return fmt::format("{}/MacOS", getBundlePath());
	}
	else
	{
		return m_bundle.subDirectory();
	}
}

/*****************************************************************************/
std::string AppBundlerMacOS::getResourcePath() const
{
	if (m_bundle.macosBundle().isAppBundle())
	{
		return fmt::format("{}/Resources", getBundlePath());
	}
	else
	{
		return m_bundle.subDirectory();
	}
}

/*****************************************************************************/
std::string AppBundlerMacOS::getFrameworksPath() const
{
	if (m_bundle.macosBundle().isAppBundle())
	{
		return fmt::format("{}/Frameworks", getBundlePath());
	}
	else
	{
		return m_bundle.subDirectory();
	}
}

/*****************************************************************************/
bool AppBundlerMacOS::changeRPathOfDependents(const std::string& inInstallNameTool, BinaryDependencyMap& inDependencyMap, const std::string& inExecutablePath) const
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
bool AppBundlerMacOS::changeRPathOfDependents(const std::string& inInstallNameTool, const std::string& inFile, const StringList& inDependencies, const std::string& inOutputFile) const
{
	if (!Commands::pathExists(inOutputFile))
		return true;

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

	if (!icon.empty())
	{
		m_iconBaseName = String::getPathBaseName(icon);

		const auto& sips = m_state.tools.sips();
		bool sipsFound = !sips.empty();

		if (String::endsWith(".png", icon) && sipsFound)
		{
			std::string outIcon = fmt::format("{}/{}.icns", m_resourcePath, m_iconBaseName);
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
				Diagnostic::warn("{}: Icon conversion from '{}' to icns requires the 'sips' command line tool.", m_inputFile, icon);
			}
		}
	}

	return true;
}

/*****************************************************************************/
bool AppBundlerMacOS::createPListAndReplaceVariables() const
{
	auto& macosBundle = m_bundle.macosBundle();

	const auto& version = m_state.workspace.version();
	auto icon = fmt::format("{}.icns", m_iconBaseName);

	auto replacePlistVariables = [&](std::string& outContent) {
		String::replaceAll(outContent, "${name}", m_bundle.name());
		String::replaceAll(outContent, "${mainExecutable}", m_mainExecutable);
		String::replaceAll(outContent, "${icon}", icon);
		String::replaceAll(outContent, "${bundleName}", macosBundle.bundleName());
		String::replaceAll(outContent, "${version}", version);
	};

	std::string tmpInfoPlist = fmt::format("{}/Info.plist.json", m_bundle.subDirectory());
	const auto& infoPropertyList = macosBundle.infoPropertyList();
	std::string infoPropertyListContent = macosBundle.infoPropertyListContent();

	if (infoPropertyListContent.empty())
	{
		if (infoPropertyList.empty())
		{
			Diagnostic::error("No info plist or plist content");
			return true;
		}

		if (String::endsWith(".plist", infoPropertyList))
		{
			if (!m_state.tools.plistConvertToJson(infoPropertyList, tmpInfoPlist))
				return false;
		}
		else if (!String::endsWith(".json", infoPropertyList))
		{
			Diagnostic::error("Unknown plist file '{}' - Must be in json or binary1 format", infoPropertyList);
			return true;
		}

		std::ifstream input(infoPropertyList);
		for (std::string line; std::getline(input, line);)
		{
			infoPropertyListContent += line + "\n";
		}
		input.close();
	}

	replacePlistVariables(infoPropertyListContent);
	std::ofstream(tmpInfoPlist) << infoPropertyListContent << std::endl;

	const std::string outInfoPropertyList = fmt::format("{}/Info.plist", m_bundlePath);
	if (!m_state.tools.plistConvertToBinary(tmpInfoPlist, outInfoPropertyList))
		return false;

	Commands::remove(tmpInfoPlist);

	return true;
}

/*****************************************************************************/
bool AppBundlerMacOS::setExecutablePaths() const
{
	auto& installNameTool = m_state.tools.installNameTool();

	for (auto p : m_state.workspace.searchPaths())
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

	StringList addedFrameworks;

	uint i = 0;
	for (auto& target : m_state.targets)
	{
		if (target->isProject())
		{
			auto& project = static_cast<const SourceTarget&>(*target);
			for (auto& framework : project.macosFrameworks())
			{
				for (auto& path : project.macosFrameworkPaths())
				{
					// Don't include System frameworks
					// TODO: maybe make an option for this? Not sure what scenarios this is needed
					if (String::startsWith("/System/Library/Frameworks", path))
						continue;

					const std::string filename = fmt::format("{}/{}.framework", path, framework);
					if (!Commands::pathExists(filename))
						continue;

					if (List::contains(addedFrameworks, framework))
						continue;

					addedFrameworks.push_back(framework);

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
	const auto& name = m_bundle.name();
	if (!macosBundle.makeDmg())
		return true;

	const auto& subDirectory = m_bundle.subDirectory();

	auto& hdiutil = m_state.tools.hdiutil();
	auto& tiffutil = m_state.tools.tiffutil();
	const std::string volumePath = fmt::format("/Volumes/{}", name);
	const std::string appPath = fmt::format("{}/{}.app", subDirectory, name);

	Commands::subprocessNoOutput({ hdiutil, "detach", fmt::format("{}/", volumePath) });

	Timer timer;

	Diagnostic::infoEllipsis("Creating the distribution disk image");

	const std::string tmpDmg = fmt::format("{}/.tmp.dmg", subDirectory);

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

	if (!Commands::subprocessNoOutput({ hdiutil, "create", "-megabytes", fmt::format("{}", dmgSize), "-fs", "HFS+", "-volname", name, tmpDmg }))
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
	const bool hasBackground = !background1x.empty() || !background2x.empty();

	if (hasBackground)
	{
		StringList cmd{ tiffutil, "-cathidpicheck" };

		if (!background1x.empty())
			cmd.push_back(background1x);

		if (!background2x.empty())
			cmd.push_back(background2x);

		cmd.emplace_back("-out");
		cmd.emplace_back(fmt::format("{}/background.tiff", backgroundPath));

		if (!Commands::subprocessNoOutput(cmd))
			return false;
	}

	if (!Commands::createDirectorySymbolicLink("/Applications", fmt::format("{}/Applications", volumePath)))
		return false;

	const auto applescriptText = PlatformFileTemplates::macosDmgApplescript(name, hasBackground);

	if (!Commands::subprocess({ m_state.tools.osascript(), "-e", applescriptText }))
		return false;

	Commands::removeRecursively(fmt::format("{}/.fseventsd", volumePath));

	if (!Commands::subprocessNoOutput({ hdiutil, "detach", fmt::format("{}/", volumePath) }))
		return false;

	const std::string outDmgPath = fmt::format("{}/{}.dmg", subDirectory, name);
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
	if (m_state.tools.signingIdentity().empty())
	{
		Diagnostic::warn("bundle '{}' was not signed - signingIdentity is not set, or was empty.", m_bundle.name());
		return true;
	}

	Timer timer;

	if (m_bundle.macosBundle().isAppBundle())
	{
		Diagnostic::infoEllipsis("Signing the application bundle");
	}
	else
	{
		Diagnostic::infoEllipsis("Signing binaries");
	}

	// TODO: Entitlements
	bool isBundle = String::endsWith(".app/Contents", m_bundlePath);

	StringList signPaths;
	StringList signLater;

	CHALET_TRY
	{
		signPaths.push_back(m_executablePath);
		if (isBundle)
		{
			signPaths.push_back(m_frameworkPath);
			signPaths.push_back(m_resourcePath);
		}

		StringList bundleExtensions{
			".app",
			".framework",
			".kext",
			".plugin",
			".docset",
			".xpc",
			".qlgenerator",
			".component",
			".saver",
			".mdimporter",
		};
		for (auto& bundlePath : signPaths)
		{
			for (const auto& entry : fs::recursive_directory_iterator(bundlePath))
			{
				auto path = entry.path().string();
				if (entry.is_regular_file() || (entry.is_directory() && String::endsWith(bundleExtensions, path)))
				{
					if (!m_state.tools.macosCodeSignFile(path))
					{
						signLater.push_back(path);
					}
				}
			}
		}

		uint signingAttempts = 3;
		for (uint i = 0; i < signingAttempts; ++i)
		{
			auto it = signLater.end();
			while (it != signLater.begin())
			{
				--it;
				auto& path = (*it);
				if (m_state.tools.macosCodeSignFile(path))
				{
					it = signLater.erase(it);
				}
			}

			if (signLater.empty())
				break;
		}

		for (auto& path : signLater)
		{
			Diagnostic::error("Failed to sign: {}", path);
		}

		if (!signLater.empty())
			return false;

		if (isBundle)
		{
			auto appPath = m_bundlePath.substr(0, m_bundlePath.size() - 9);
			if (!m_state.tools.macosCodeSignFile(appPath))
			{
				Diagnostic::error("Failed to sign: {}", appPath);
				return false;
			}
		}

		Diagnostic::printDone(timer.asString());

		return true;
	}
	CHALET_CATCH(const std::exception& err)
	{
		CHALET_EXCEPT_ERROR(err.what());
		return false;
	}
}

/*****************************************************************************/
bool AppBundlerMacOS::signDmgImage(const std::string& inPath) const
{
	if (m_state.tools.signingIdentity().empty())
	{
		Diagnostic::warn("dmg '{}' was not signed - signingIdentity is not set, or was empty.", inPath);
		return true;
	}

	Timer timer;
	Diagnostic::infoEllipsis("Signing the disk image");

	if (!m_state.tools.macosCodeSignDiskImage(inPath))
	{
		Diagnostic::error("Failed to sign: {}", inPath);
		return false;
	}

	Diagnostic::printDone(timer.asString());

	return true;
}
}
