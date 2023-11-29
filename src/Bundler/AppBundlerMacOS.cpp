/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Bundler/AppBundlerMacOS.hpp"

#include "FileTemplates/PlatformFileTemplates.hpp"

#include "Bundler/MacosCodeSignOptions.hpp"
#include "Cache/SourceCache.hpp"
#include "Cache/WorkspaceCache.hpp"
#include "Core/CommandLineInputs.hpp"
#include "Process/Environment.hpp"
#include "Process/Process.hpp"
#include "State/AncillaryTools.hpp"
#include "State/BuildPaths.hpp"
#include "State/BuildState.hpp"
#include "State/CompilerTools.hpp"
#include "State/Distribution/BundleTarget.hpp"
#include "State/Target/IBuildTarget.hpp"
#include "State/Target/SourceTarget.hpp"
#include "State/TargetMetadata.hpp"
#include "State/WorkspaceEnvironment.hpp"
#include "System/Files.hpp"
#include "Terminal/Output.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"
#include "Utility/Timer.hpp"
#include "Utility/Version.hpp"
#include "Json/JsonComments.hpp"
#include "Json/JsonFile.hpp"

namespace chalet
{
/*****************************************************************************/
AppBundlerMacOS::AppBundlerMacOS(BuildState& inState, const BundleTarget& inBundle, BinaryDependencyMap& inDependencyMap) :
	IAppBundler(inState, inBundle, inDependencyMap)
{
}

/*****************************************************************************/
bool AppBundlerMacOS::initializeState()
{
#if defined(CHALET_MACOS)
	m_bundlePath = getBundlePath();
	m_frameworksPath = getFrameworksPath();
	m_resourcePath = getResourcePath();
	m_executablePath = getExecutablePath();

	m_infoFile = getPlistFile();
	m_entitlementsFile = getEntitlementsFilePath();

	if (!getMainExecutable(m_mainExecutable))
		return false; // No executable. we don't care

	return true;
#else
	return false;
#endif
}

/*****************************************************************************/
const std::string& AppBundlerMacOS::mainExecutable() const noexcept
{
	return m_mainExecutable;
}

/*****************************************************************************/
void AppBundlerMacOS::setOutputDirectory(const std::string& inPath) const
{
#if defined(CHALET_MACOS)
	m_outputDirectory = inPath;
#else
	UNUSED(inPath);
#endif
}

/*****************************************************************************/
bool AppBundlerMacOS::removeOldFiles()
{
	return true;
}

/*****************************************************************************/
bool AppBundlerMacOS::quickBundleForPlatform()
{
#if defined(CHALET_MACOS)
	// If we got this far, the app bundle was built through Xcode
	// so we only need to copy it

	if (!initializeState())
		return false;

	auto appPath = String::getPathFolder(m_bundlePath);
	if (appPath.empty())
		return false;

	auto appName = String::getPathFilename(appPath);
	auto outputFolder = String::getPathFolder(appPath);

	auto& buildOutputDir = m_state.paths.buildOutputDir();
	if (!Files::copy(fmt::format("{}/{}", buildOutputDir, appName), outputFolder))
	{
		Diagnostic::error("There was an problem copying {} to the output directory ({})", appName, outputFolder);
		return false;
	}

	return true;
#else
	return false;
#endif
}

/*****************************************************************************/
bool AppBundlerMacOS::bundleForPlatform()
{
#if defined(CHALET_MACOS)
	if (!initializeState())
		return true; // No executable. we don't care

	{
		auto executables = getAllExecutables();
		m_executableOutputPaths.clear();
		for (auto& executable : executables)
		{
			m_executableOutputPaths.emplace_back(fmt::format("{}/{}", m_executablePath, executable));
		}
	}

	if (!Files::pathExists(m_frameworksPath))
		Files::makeDirectory(m_frameworksPath);

	auto& installNameTool = m_state.tools.installNameTool();
	if (m_bundle.updateRPaths())
	{
		if (!changeRPathOfDependents(installNameTool, m_dependencyMap, m_frameworksPath))
			return false;

		if (!changeRPathOfDependents(installNameTool, m_dependencyMap, m_executablePath))
			return false;
	}

	/*for (auto& executable : m_executableOutputPaths)
	{
		if (!Process::run({ installNameTool, "-add_rpath", "@executable_path/.", executable }))
			return false;
	}*/

	// treat it like linux/windows
	if (m_bundle.macosBundleType() == MacOSBundleType::None)
	{
		if (!signAppBundle())
			return false;

		Output::msgAction("Succeeded", m_outputDirectory);
	}
	else
	{
		if (!createBundleIconFromXcassets())
			return false;

		if (!createInfoPropertyListAndReplaceVariables(m_infoFile))
			return false;

		if (!createEntitlementsPropertyList(m_entitlementsFile))
			return false;

		if (!setExecutablePaths())
			return false;

		if (!signAppBundle())
			return false;

		if (Files::pathExists(m_entitlementsFile))
			Files::remove(m_entitlementsFile);

		Output::msgAction("Succeeded", String::getPathFolder(m_bundlePath));
	}

	return true;
#else
	return false;
#endif
}

/*****************************************************************************/
std::string AppBundlerMacOS::getBundlePath() const
{
#if defined(CHALET_MACOS)
	if (m_outputDirectory.empty())
		setOutputDirectory(m_bundle.subdirectory());

	if (m_bundle.isMacosAppBundle())
		return fmt::format("{}/{}.app/Contents", m_outputDirectory, m_bundle.name());
	else
		return m_outputDirectory;
#else
	return std::string();
#endif
}

/*****************************************************************************/
std::string AppBundlerMacOS::getExecutablePath() const
{
#if defined(CHALET_MACOS)
	if (m_bundle.isMacosAppBundle())
		return fmt::format("{}/MacOS", getBundlePath());
	else
		return m_outputDirectory;
#else
	return std::string();
#endif
}

/*****************************************************************************/
std::string AppBundlerMacOS::getResourcePath() const
{
#if defined(CHALET_MACOS)
	if (m_bundle.isMacosAppBundle())
		return fmt::format("{}/Resources", getBundlePath());
	else
		return m_outputDirectory;
#else
	return std::string();
#endif
}

/*****************************************************************************/
std::string AppBundlerMacOS::getFrameworksPath() const
{
#if defined(CHALET_MACOS)
	if (m_bundle.isMacosAppBundle())
		return fmt::format("{}/Frameworks", getBundlePath());
	else
		return m_outputDirectory;
#else
	return std::string();
#endif
}

/*****************************************************************************/
std::string AppBundlerMacOS::getResolvedIconName() const
{
	auto& bundleIcon = m_bundle.macosBundleIcon();
	std::string icon;
	if (String::endsWith(".icns", bundleIcon))
		icon = String::getPathBaseName(bundleIcon);
	else
		icon = "AppIcon";

	return icon;
}

/*****************************************************************************/
bool AppBundlerMacOS::changeRPathOfDependents(const std::string& inInstallNameTool, const BinaryDependencyMap& inDependencyMap, const std::string& inExecutablePath) const
{
#if defined(CHALET_MACOS)
	for (auto& [file, dependencies] : inDependencyMap)
	{
		auto filename = String::getPathFilename(file);
		const auto outputFile = fmt::format("{}/{}", inExecutablePath, filename);

		if (!changeRPathOfDependents(inInstallNameTool, filename, dependencies, outputFile))
			return false;
	}

	return true;
#else
	UNUSED(inInstallNameTool, inDependencyMap, inExecutablePath);
	return false;
#endif
}

/*****************************************************************************/
bool AppBundlerMacOS::changeRPathOfDependents(const std::string& inInstallNameTool, const std::string& inFile, const StringList& inDependencies, const std::string& inOutputFile) const
{
#if defined(CHALET_MACOS)
	if (!Files::pathExists(inOutputFile))
		return true;

	if (inDependencies.size() > 0)
	{
		if (!Process::run({ inInstallNameTool, "-id", fmt::format("@rpath/{}", inFile), inOutputFile }))
		{
			Diagnostic::error("install_name_tool error");
			return false;
		}
	}

	StringList exclusions{ "/usr/lib/" };
	for (auto& dep : inDependencies)
	{
		if (String::startsWith(exclusions, dep))
			continue;

		auto depFile = String::getPathFilename(dep);
		if (!Process::run({ inInstallNameTool, "-change", dep, fmt::format("@rpath/{}", depFile), inOutputFile }))
		{
			Diagnostic::error("install_name_tool error");
			return false;
		}

		// For dylibs linked w/o .a files, they get assigned "@executable_path/../Frameworks/"
		//   so we need to attempt to update them as well
		auto depFrameworkPath = fmt::format("@executable_path/../Frameworks/{}", depFile);
		if (!Process::run({ inInstallNameTool, "-change", depFrameworkPath, fmt::format("@rpath/{}", depFile), inOutputFile }))
		{
			Diagnostic::error("install_name_tool error");
			return false;
		}
	}

	return true;
#else
	UNUSED(inInstallNameTool, inFile, inDependencies, inOutputFile);
	return false;
#endif
}

/*****************************************************************************/
bool AppBundlerMacOS::createAssetsXcassets(const std::string& inOutPath)
{
#if defined(CHALET_MACOS)
	const auto& icon = m_bundle.macosBundleIcon();
	auto& sourceCache = m_state.cache.file().sources();

	// calls to sips take a significant chunk of time,
	//   so we only do this if the icon doesn't exist or has changed
	//
	if (!sourceCache.fileChangedOrDoesNotExist(icon, inOutPath))
		return true;

	if (!Files::pathExists(inOutPath))
		Files::makeDirectory(inOutPath);

	auto accentColorPath = fmt::format("{}/AccentColor.colorset", inOutPath);
	auto appIconPath = fmt::format("{}/AppIcon.appiconset", inOutPath);

	if (!Files::pathExists(accentColorPath))
		Files::makeDirectory(accentColorPath);

	if (!Files::pathExists(appIconPath))
		Files::makeDirectory(appIconPath);

	Json root = R"json({
		"info" : { "author" : "xcode", "version" : 1 }
	})json"_ojson;
	std::ofstream(fmt::format("{}/Contents.json", inOutPath)) << root.dump(1, '\t') << std::endl;

	Json accentColorJson = R"json({
		"colors" : [{ "idiom" : "universal" }],
		"info" : { "author" : "xcode", "version" : 1 }
	})json"_ojson;
	std::ofstream(fmt::format("{}/Contents.json", accentColorPath)) << accentColorJson.dump(1, '\t') << std::endl;

	Json appIconJson = R"json({
		"images" : [],
		"info" : { "author" : "xcode", "version" : 1 }
	})json"_ojson;

	const auto& sips = m_state.tools.sips();

	auto addIdiom = [&appIconJson, &appIconPath, &icon, &sips](i32 scale, i32 size) {
		Json out = Json::object();

		if (!icon.empty() && !sips.empty())
		{
			auto baseName = String::getPathBaseName(icon);
			auto ext = String::getPathSuffix(icon);
			i32 imageSize = scale * size;
			auto outIcon = fmt::format("{}/{}-{}@{}x.{}", appIconPath, baseName, size, scale, ext);
			// -Z 32 glfw.icns --out glfw-32.icns
			if (Process::runNoOutput({ sips, "-Z", std::to_string(imageSize), icon, "--out", outIcon }))
			{
				out["filename"] = String::getPathFilename(outIcon);
			}
		}

		out["idiom"] = "mac"; // support ios/watchos here
		out["scale"] = fmt::format("{}x", scale);
		out["size"] = fmt::format("{}x{}", size, size);
		appIconJson["images"].emplace_back(std::move(out));
	};

	std::vector<i32> sizes{ 16, 32, 128, 256, 512 };
	for (i32 size : sizes)
	{
		addIdiom(1, size);
		addIdiom(2, size);
	}

	std::ofstream(fmt::format("{}/Contents.json", appIconPath))
		<< appIconJson.dump(1, '\t') << std::endl;

	return true;

#else
	UNUSED(inOutPath);
	return false;
#endif
}

/*****************************************************************************/
bool AppBundlerMacOS::createInfoPropertyListAndReplaceVariables(const std::string& inOutFile, Json* outJson) const
{
#if defined(CHALET_MACOS)
	if (inOutFile.empty())
		return false;

	// const auto& version = m_state.workspace.version();

	auto replacePlistVariables = [this](std::string& outContent) {
		String::replaceAll(outContent, "${name}", m_bundle.name());
		String::replaceAll(outContent, "${mainExecutable}", m_mainExecutable);
		String::replaceAll(outContent, "${icon}", getResolvedIconName());
		String::replaceAll(outContent, "${bundleName}", m_bundle.macosBundleName());
		String::replaceAll(outContent, "${osTargetVersion}", m_state.inputs.osTargetVersion());

		// TODO: This uses the workspace version, but should be the same version as the mainExecutable
		String::replaceAll(outContent, "${version}", m_state.workspace.metadata().versionString());
	};

	std::string tmpPlist = fmt::format("{}.json", inOutFile);
	std::string infoPropertyList = m_bundle.macosBundleInfoPropertyList();
	std::string infoPropertyListContent = m_bundle.macosBundleInfoPropertyListContent();

	if (infoPropertyListContent.empty())
	{
		if (infoPropertyList.empty())
		{
			Diagnostic::error("No info plist or plist content");
			return true;
		}

		if (String::endsWith(".plist", infoPropertyList))
		{
			if (!m_state.tools.plistConvertToJson(infoPropertyList, tmpPlist))
				return false;

			infoPropertyList = tmpPlist;
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
	std::ofstream(tmpPlist) << infoPropertyListContent << std::endl;

	if (outJson != nullptr)
	{
		if (!JsonComments::parse(*outJson, tmpPlist))
			return false;
	}

	if (!m_state.tools.plistConvertToBinary(tmpPlist, inOutFile))
		return false;

	Files::remove(tmpPlist);
	return true;
#else
	UNUSED(inOutFile, outJson);
	return false;
#endif
}

/*****************************************************************************/
bool AppBundlerMacOS::createEntitlementsPropertyList(const std::string& inOutFile) const
{
#if defined(CHALET_MACOS)
	if (inOutFile.empty())
		return true;

	std::string tmpPlist = fmt::format("{}.json", inOutFile);
	std::string entitlements = m_bundle.macosBundleEntitlementsPropertyList();
	std::string entitlementsContent = m_bundle.macosBundleEntitlementsPropertyListContent();

	// No entitlements
	if (entitlements.empty() && entitlementsContent.empty())
		return true;

	if (entitlementsContent.empty())
	{
		if (String::endsWith({ ".plist", ".xml" }, entitlements))
		{
			if (!m_state.tools.plistConvertToJson(entitlements, tmpPlist))
				return false;

			entitlements = tmpPlist;
		}
		else if (!String::endsWith(".json", entitlements))
		{
			Diagnostic::error("Unknown plist file '{}' - Must be in json or xml1 format", entitlements);
			return true;
		}

		std::ifstream input(entitlements);
		for (std::string line; std::getline(input, line);)
		{
			entitlementsContent += line + "\n";
		}
		input.close();
	}

	std::ofstream(tmpPlist) << entitlementsContent << std::endl;

	if (!m_state.tools.plistConvertToXml(tmpPlist, inOutFile))
		return false;

	Files::remove(tmpPlist);
	return true;
#else
	UNUSED(inOutFile);
	return false;
#endif
}

#if defined(CHALET_MACOS)
/*****************************************************************************/
std::string AppBundlerMacOS::getPlistFile() const
{
	return fmt::format("{}/Info.plist", m_bundlePath);
}

/*****************************************************************************/
std::string AppBundlerMacOS::getEntitlementsFilePath() const
{
	const auto& entitlements = m_bundle.macosBundleEntitlementsPropertyList();
	const auto& entitlementsContent = m_bundle.macosBundleEntitlementsPropertyListContent();

	// No entitlements
	if (!entitlements.empty() || !entitlementsContent.empty())
		return fmt::format("{}/App.entitlements", m_outputDirectory);

	return std::string();
}

/*****************************************************************************/
bool AppBundlerMacOS::createBundleIcon()
{
	const auto& icon = m_bundle.macosBundleIcon();

	if (icon.empty())
		return true;

	Timer timer;

	if (!Output::showCommands())
		Diagnostic::stepInfoEllipsis("Creating the bundle icon from '{}'", icon);

	const auto& sips = m_state.tools.sips();
	bool sipsFound = !sips.empty();

	if (String::endsWith(".png", icon) && sipsFound)
	{
		auto iconBaseName = String::getPathBaseName(icon);
		auto outIcon = fmt::format("{}/{}.icns", m_resourcePath, iconBaseName);
		if (!Process::runMinimalOutput({ sips, "-s", "format", "icns", icon, "--out", outIcon }))
			return false;
	}
	else if (String::endsWith(".icns", icon))
	{
		auto copyFunc = Output::showCommands() ? Files::copy : Files::copySilent;
		if (!copyFunc(icon, m_resourcePath, fs::copy_options::overwrite_existing))
			return false;
	}
	else
	{
		if (!icon.empty() && !sipsFound)
		{
			auto& inputFile = m_state.inputs.inputFile();
			Diagnostic::warn("{}: Icon conversion from '{}' to icns requires the 'sips' command line tool.", inputFile, icon);
		}
	}

	if (!Output::showCommands())
		Diagnostic::printDone(timer.asString());

	return true;
}

/*****************************************************************************/
bool AppBundlerMacOS::createBundleIconFromXcassets()
{
	const auto& icon = m_bundle.macosBundleIcon();
	if (icon.empty())
		return true;

	if (String::endsWith(".icns", icon))
		return createBundleIcon();

	Timer timer;

	auto objDir = m_state.paths.bundleObjDir(m_bundle.name());
	if (!Files::pathExists(objDir))
		Files::makeDirectory(objDir);

	bool iconIsXcassets = String::endsWith(".xcassets", icon);

	auto assetsPath = fmt::format("{}/Assets.xcassets", objDir);
	if (iconIsXcassets)
		assetsPath = icon;

	auto usingCommandLineTools = Files::isUsingAppleCommandLineTools();

	auto actool = Files::which("actool");
	if (actool.empty() || usingCommandLineTools)
	{
		auto& inputFile = m_state.inputs.inputFile();
		if (iconIsXcassets)
		{
			if (usingCommandLineTools)
				Diagnostic::error("{}: Icon conversion from '{}' to icns requires the 'actool' cli tool from Xcode.", inputFile, icon);
			else
				Diagnostic::error("{}: Icon conversion from '{}' to icns requires the 'actool' cli tool.", inputFile, icon);
			return false;
		}

		if (!usingCommandLineTools)
		{
			Diagnostic::warn("Could not find 'actool' required to create an icns from an asset catalog. Falling back to 'sips' method.");
		}
		// If actool is not found or using command line tools, make the bundle icon the old way
		return createBundleIcon();
	}

	if (iconIsXcassets)
		Diagnostic::stepInfoEllipsis("Using the asset catalog: '{}'", icon);
	else
		Diagnostic::stepInfoEllipsis("Creating an asset catalog from '{}'", icon);

	if (!createAssetsXcassets(assetsPath))
	{
		Diagnostic::error("Could not create '{}' for application icon.", assetsPath);
		return false;
	}

	auto tempPlist = fmt::format("{}/assetcatalog_generated_info.plist", objDir);

	bool result = Process::runNoOutput({
		actool,
		"--output-format",
		"human-readable-text",
		"--notices",
		"--warnings",
		"--export-dependency-info",
		fmt::format("{}/assetcatalog_dependencies", objDir),
		"--output-partial-info-plist",
		tempPlist,
		"--app-icon",
		"AppIcon",
		"--enable-on-demand-resources",
		"YES",
		"--development-region",
		"en",
		"--target-device",
		"mac",
		"--minimum-deployment-target",
		m_state.inputs.osTargetVersion(),
		"--platform",
		m_state.inputs.osTargetName(),
		"--compile",
		m_resourcePath,
		assetsPath,
	});

	if (result)
	{
		Diagnostic::printDone(timer.asString());
	}
	else
	{
		Diagnostic::error("There was a problem creating the application bundle icon.", assetsPath);
	}

	return result;
}

/*****************************************************************************/
bool AppBundlerMacOS::setExecutablePaths() const
{
	auto& installNameTool = m_state.tools.installNameTool();

	// install_name_tool
	for (auto& executable : m_executableOutputPaths)
	{
		if (!Process::runNoOutput({ installNameTool, "-add_rpath", "@executable_path/../MacOS", executable }))
			return false;

		if (!Process::runNoOutput({ installNameTool, "-add_rpath", "@executable_path/../Frameworks", executable }))
			return false;

		if (!Process::runNoOutput({ installNameTool, "-add_rpath", "@executable_path/../Resources", executable }))
			return false;
	}

	StringList addedFrameworks;

	for (auto& target : m_state.targets)
	{
		if (target->isSources())
		{
			auto& project = static_cast<const SourceTarget&>(*target);

			for (auto& framework : project.appleFrameworks())
			{
				for (auto& path : project.appleFrameworkPaths())
				{
					// Don't include System frameworks
					// TODO: maybe make an option for this? Not sure what scenarios this is needed
					if (String::startsWith("/System/Library/Frameworks", path))
						continue;

					const std::string filename = fmt::format("{}/{}.framework", path, framework);
					if (!Files::pathExists(filename))
						continue;

					if (List::contains(addedFrameworks, framework))
						continue;

					addedFrameworks.push_back(framework);

					if (!Files::copy(filename, m_frameworksPath, fs::copy_options::skip_existing))
						return false;

					const auto resolvedFramework = fmt::format("{}/{}.framework", m_frameworksPath, framework);

					for (auto& executable : m_executableOutputPaths)
					{
						if (!Process::run({ installNameTool, "-change", resolvedFramework, fmt::format("@rpath/{}", filename), executable }))
							return false;
					}

					break;
				}
			}
		}
	}

	return true;
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

	if (m_bundle.isMacosAppBundle())
	{
		Diagnostic::stepInfoEllipsis("Signing the application bundle");
	}
	else
	{
		Diagnostic::stepInfoEllipsis("Signing binaries");
	}

	bool isBundle = String::endsWith(".app/Contents", m_bundlePath);

	// TODO: provisioning profile (includes the entitlements section)

	MacosCodeSignOptions entitlementOptions;
	entitlementOptions.entitlementsFile = m_entitlementsFile;
	entitlementOptions.hardenedRuntime = true;

	StringList signPaths;
	StringList signLater;

	CHALET_TRY
	{
		signPaths.push_back(m_executablePath);
		if (isBundle)
		{
			signPaths.push_back(m_frameworksPath);
			signPaths.push_back(m_resourcePath);
		}

		StringList bundleExtensions{
			".app",
			Files::getPlatformFrameworkExtension(),
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
					if (!m_state.tools.macosCodeSignFile(path, entitlementOptions))
					{
						signLater.push_back(path);
					}
				}
			}
		}

		u32 signingAttempts = 3;
		for (u32 i = 0; i < signingAttempts; ++i)
		{
			auto it = signLater.end();
			while (it != signLater.begin())
			{
				--it;
				auto& path = (*it);
				if (m_state.tools.macosCodeSignFile(path, entitlementOptions))
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
			if (!m_state.tools.macosCodeSignFile(appPath, entitlementOptions))
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
#endif
}
