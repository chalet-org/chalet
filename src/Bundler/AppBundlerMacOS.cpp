/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Bundler/AppBundlerMacOS.hpp"

#include "FileTemplates/PlatformFileTemplates.hpp"

#include "Bundler/MacosCodeSignOptions.hpp"
#include "State/AncillaryTools.hpp"
#include "State/BuildPaths.hpp"
#include "State/BuildState.hpp"
#include "State/CompilerTools.hpp"
#include "State/Distribution/BundleTarget.hpp"
#include "State/Target/IBuildTarget.hpp"
#include "State/Target/SourceTarget.hpp"
#include "State/WorkspaceEnvironment.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Environment.hpp"
#include "Terminal/Output.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"
#include "Utility/Timer.hpp"

namespace chalet
{
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
#if defined(CHALET_MACOS)
	m_bundlePath = getBundlePath();
	m_frameworksPath = getFrameworksPath();
	m_resourcePath = getResourcePath();
	m_executablePath = getExecutablePath();

	m_entitlementsFile = getEntitlementsFilePath();

	if (!getMainExecutable(m_mainExecutable))
		return true; // No executable. we don't care

	{
		auto executables = getAllExecutables();
		m_executableOutputPaths.clear();
		for (auto& executable : executables)
		{
			m_executableOutputPaths.emplace_back(fmt::format("{}/{}", m_executablePath, executable));
		}
	}

	if (!Commands::pathExists(m_frameworksPath))
		Commands::makeDirectory(m_frameworksPath);

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
		if (!Commands::subprocess({ installNameTool, "-add_rpath", "@executable_path/.", executable }))
			return false;
	}*/

	// treat it like linux/windows
	if (m_bundle.macosBundleType() == MacOSBundleType::None)
	{
		return signAppBundle();
	}
	else
	{
		// TODO: Generalized version of this in AppBundler

		if (!createBundleIcon())
			return false;

		if (!createInfoPropertyListAndReplaceVariables())
			return false;

		if (!createEntitlementsPropertyList())
			return false;

		if (!setExecutablePaths())
			return false;

		if (!signAppBundle())
			return false;

		if (Commands::pathExists(m_entitlementsFile))
			Commands::remove(m_entitlementsFile);

		return true;
	}
#else
	return false;
#endif
}

/*****************************************************************************/
std::string AppBundlerMacOS::getBundlePath() const
{
	const auto& subdirectory = m_bundle.subdirectory();
#if defined(CHALET_MACOS)
	if (m_bundle.isMacosAppBundle())
		return fmt::format("{}/{}.app/Contents", subdirectory, m_bundle.name());
	else
#endif
		return subdirectory;
}

/*****************************************************************************/
std::string AppBundlerMacOS::getExecutablePath() const
{
#if defined(CHALET_MACOS)
	if (m_bundle.isMacosAppBundle())
		return fmt::format("{}/MacOS", getBundlePath());
	else
#endif
		return m_bundle.subdirectory();
}

/*****************************************************************************/
std::string AppBundlerMacOS::getResourcePath() const
{
#if defined(CHALET_MACOS)
	if (m_bundle.isMacosAppBundle())
		return fmt::format("{}/Resources", getBundlePath());
	else
#endif
		return m_bundle.subdirectory();
}

/*****************************************************************************/
std::string AppBundlerMacOS::getFrameworksPath() const
{
#if defined(CHALET_MACOS)
	if (m_bundle.isMacosAppBundle())
		return fmt::format("{}/Frameworks", getBundlePath());
	else
#endif
		return m_bundle.subdirectory();
}

/*****************************************************************************/
bool AppBundlerMacOS::changeRPathOfDependents(const std::string& inInstallNameTool, const BinaryDependencyMap& inDependencyMap, const std::string& inExecutablePath) const
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

	StringList exclusions{ "/usr/lib/" };
	for (auto& dep : inDependencies)
	{
		if (String::startsWith(exclusions, dep))
			continue;

		auto depFile = String::getPathFilename(dep);
		if (!Commands::subprocess({ inInstallNameTool, "-change", dep, fmt::format("@rpath/{}", depFile), inOutputFile }))
		{
			Diagnostic::error("install_name_tool error");
			return false;
		}

		// For dylibs linked w/o .a files, they get assigned "@executable_path/../Frameworks/"
		//   so we need to attempt to update them as well
		auto depFrameworkPath = fmt::format("@executable_path/../Frameworks/{}", depFile);
		if (!Commands::subprocess({ inInstallNameTool, "-change", depFrameworkPath, fmt::format("@rpath/{}", depFile), inOutputFile }))
		{
			Diagnostic::error("install_name_tool error");
			return false;
		}
	}

	return true;
}

/*****************************************************************************/
std::string AppBundlerMacOS::getEntitlementsFilePath() const
{
#if defined(CHALET_MACOS)
	const auto& entitlements = m_bundle.macosBundleEntitlementsPropertyList();
	const auto& entitlementsContent = m_bundle.macosBundleEntitlementsPropertyListContent();

	// No entitlements
	if (!entitlements.empty() || !entitlementsContent.empty())
		return fmt::format("{}/Entitlements.plist", m_bundle.subdirectory());
#endif

	return std::string();
}

/*****************************************************************************/
bool AppBundlerMacOS::createBundleIcon()
{
#if defined(CHALET_MACOS)
	const auto& icon = m_bundle.macosBundleIcon();

	if (!icon.empty())
	{
		m_iconBaseName = String::getPathBaseName(icon);

		const auto& sips = m_state.tools.sips();
		bool sipsFound = !sips.empty();

		if (String::endsWith(".png", icon) && sipsFound)
		{
			std::string outIcon = fmt::format("{}/{}.icns", m_resourcePath, m_iconBaseName);
			if (!Commands::subprocessMinimalOutput({ sips, "-s", "format", "icns", icon, "--out", outIcon }))
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
#endif

	return true;
}

/*****************************************************************************/
bool AppBundlerMacOS::createInfoPropertyListAndReplaceVariables() const
{
#if defined(CHALET_MACOS)
	// const auto& version = m_state.workspace.version();
	auto icon = fmt::format("{}.icns", m_iconBaseName);

	auto replacePlistVariables = [&](std::string& outContent) {
		String::replaceAll(outContent, "${name}", m_bundle.name());
		String::replaceAll(outContent, "${mainExecutable}", m_mainExecutable);
		String::replaceAll(outContent, "${icon}", icon);
		String::replaceAll(outContent, "${bundleName}", m_bundle.macosBundleName());
		// String::replaceAll(outContent, "${version}", version);
	};

	std::string tmpPlist = fmt::format("{}/Info.plist.json", m_bundle.subdirectory());
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

	const std::string outInfoPropertyList = fmt::format("{}/Info.plist", m_bundlePath);
	if (!m_state.tools.plistConvertToBinary(tmpPlist, outInfoPropertyList))
		return false;

	Commands::remove(tmpPlist);
#endif

	return true;
}

/*****************************************************************************/
bool AppBundlerMacOS::createEntitlementsPropertyList() const
{
#if defined(CHALET_MACOS)
	std::string tmpPlist = fmt::format("{}.json", m_entitlementsFile);
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

	if (!m_state.tools.plistConvertToXml(tmpPlist, m_entitlementsFile))
		return false;

	Commands::remove(tmpPlist);
#endif

	return true;
}

/*****************************************************************************/
bool AppBundlerMacOS::setExecutablePaths() const
{
	auto& installNameTool = m_state.tools.installNameTool();

	// install_name_tool
	for (auto& executable : m_executableOutputPaths)
	{
		if (!Commands::subprocess({ installNameTool, "-add_rpath", "@executable_path/../MacOS", executable }))
			return false;

		if (!Commands::subprocess({ installNameTool, "-add_rpath", "@executable_path/../Frameworks", executable }))
			return false;

		if (!Commands::subprocess({ installNameTool, "-add_rpath", "@executable_path/../Resources", executable }))
			return false;
	}

	StringList addedFrameworks;

	uint i = 0;
	for (auto& target : m_state.targets)
	{
		if (target->isSources())
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

					if (!Commands::copy(filename, m_frameworksPath, fs::copy_options::skip_existing))
						return false;

					const auto resolvedFramework = fmt::format("{}/{}.framework", m_frameworksPath, framework);

					for (auto& executable : m_executableOutputPaths)
					{
						if (!Commands::subprocess({ installNameTool, "-change", resolvedFramework, fmt::format("@rpath/{}", filename), executable }))
							return false;
					}

					++i;

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

#if defined(CHALET_MACOS)
	if (m_bundle.isMacosAppBundle())
	{
		Diagnostic::stepInfoEllipsis("Signing the application bundle");
	}
	else
#endif
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
					if (!m_state.tools.macosCodeSignFile(path, entitlementOptions))
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
}
