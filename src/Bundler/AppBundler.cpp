/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Bundler/AppBundler.hpp"

#include "Builder/ScriptRunner.hpp"
#include "Bundler/IAppBundler.hpp"
#include "Bundler/MacosDiskImageCreator.hpp"
#include "Bundler/WindowsNullsoftInstallerRunner.hpp"
#include "Bundler/ZipArchiver.hpp"
#include "Core/CommandLineInputs.hpp"
#include "State/BuildInfo.hpp"
#include "State/BuildPaths.hpp"
#include "State/BuildState.hpp"
#include "State/Distribution/BundleArchiveTarget.hpp"
#include "State/Distribution/BundleTarget.hpp"
#include "State/Distribution/MacosDiskImageTarget.hpp"
#include "State/Distribution/ScriptDistTarget.hpp"
#include "State/Distribution/WindowsNullsoftInstallerTarget.hpp"
#include "State/StatePrototype.hpp"
#include "State/Target/SourceTarget.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Environment.hpp"
#include "Terminal/Output.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"
#include "Utility/Timer.hpp"

namespace chalet
{
/*****************************************************************************/
AppBundler::AppBundler(const CommandLineInputs& inInputs, StatePrototype& inPrototype) :
	m_inputs(inInputs),
	m_prototype(inPrototype),
	m_dependencyMap(m_prototype.tools)
{
}

/*****************************************************************************/
bool AppBundler::runBuilds()
{
	// Build all required configurations
	m_detectedArch = m_inputs.targetArchitecture().empty() ? "auto" : m_inputs.targetArchitecture();

	auto makeState = [&](std::string arch, const std::string& inConfig) {
		auto configName = fmt::format("{}_{}", arch, inConfig);
		if (m_states.find(configName) == m_states.end())
		{
			CommandLineInputs inputs = m_inputs;
			inputs.setBuildConfiguration(std::string(inConfig));
			inputs.setTargetArchitecture(arch);
			auto state = std::make_unique<BuildState>(std::move(inputs), m_prototype);

			m_states.emplace(configName, std::move(state));
		}
	};

	StringList arches;
	for (auto& target : m_prototype.distribution)
	{
		if (target->isDistributionBundle())
		{
			auto& bundle = static_cast<BundleTarget&>(*target);
			makeState(m_detectedArch, bundle.configuration());
		}
	}

	// BuildState* stateArchA = nullptr;
	for (auto& [config, state] : m_states)
	{
		if (!state->initialize())
			return false;

		if (!state->doBuild(Route::Build, false))
			return false;
	}

	return true;
}

/*****************************************************************************/
bool AppBundler::run(const DistTarget& inTarget)
{
	if (inTarget->isDistributionBundle())
	{
		auto& bundle = static_cast<BundleTarget&>(*inTarget);

		if (!bundle.description().empty())
		{
			Output::msgTargetDescription(bundle.description(), Output::theme().header);
		}
		else
		{
#if defined(CHALET_MACOS)
			if (bundle.isMacosAppBundle())
				Output::msgTargetOfType("Bundle", fmt::format("{}.{}", bundle.name(), bundle.macosBundleExtension()), Output::theme().header);
			else
#elif defined(CHALET_LINUX)
			if (bundle.hasLinuxDesktopEntry())
				Output::msgTargetOfType("Bundle", fmt::format("{}.desktop", bundle.name(), bundle.macosBundleExtension()), Output::theme().header);
			else
#endif
				Output::msgTargetOfType("Bundle", bundle.name(), Output::theme().header);
		}

		Output::lineBreak();

		chalet_assert(!bundle.configuration().empty(), "State not initialized");

		BuildState* buildState = nullptr;
		{
			buildState = getBuildState(bundle.configuration());
			if (buildState == nullptr)
				return false;
		}

		auto bundler = IAppBundler::make(*buildState, bundle, m_dependencyMap, m_inputs.inputFile());
		if (!removeOldFiles(*bundler))
		{
			Diagnostic::error("There was an error removing the previous distribution bundle for: {}", inTarget->name());
			return false;
		}

		if (!bundle.initialize(*buildState))
			return false;

		if (!gatherDependencies(bundle, *buildState))
			return false;

		if (!runBundleTarget(*bundler, *buildState))
			return false;
	}
	else if (inTarget->isScript())
	{
		Timer buildTimer;

		if (!runScriptTarget(static_cast<const ScriptDistTarget&>(*inTarget)))
			return false;

		auto res = buildTimer.stop();
		if (res > 0 && Output::showBenchmarks())
		{
			Output::printInfo(fmt::format("   Time: {}", buildTimer.asString()));
		}
	}
	else if (inTarget->isArchive())
	{
		if (!runArchiveTarget(static_cast<const BundleArchiveTarget&>(*inTarget)))
			return false;
	}
	else if (inTarget->isMacosDiskImage())
	{
		if (!runMacosDiskImageTarget(static_cast<const MacosDiskImageTarget&>(*inTarget)))
			return false;
	}
	else if (inTarget->isWindowsNullsoftInstaller())
	{
		if (!runWindowsNullsoftInstallerTarget(static_cast<const WindowsNullsoftInstallerTarget&>(*inTarget)))
			return false;
	}

	Output::lineBreak();

	return true;
}

/*****************************************************************************/
BuildState* AppBundler::getBuildState(const std::string& inBuildConfiguration) const
{
	BuildState* ret = nullptr;
	for (auto& [config, state] : m_states)
	{
		auto configName = fmt::format("{}_{}", m_detectedArch, inBuildConfiguration);
		if (String::equals(configName, config))
		{
			ret = state.get();
			break;
		}
	}

	chalet_assert(ret != nullptr, "State not initialized");
	if (ret == nullptr)
	{
		Diagnostic::error("Arch and/or build configuration '{}' not detected.", inBuildConfiguration);
		return nullptr;
	}

	return ret;
}

/*****************************************************************************/
bool AppBundler::runBundleTarget(IAppBundler& inBundler, BuildState& inState)
{
	auto& bundle = inBundler.bundle();
	const auto& buildTargets = inBundler.bundle().buildTargets();

	const auto bundlePath = inBundler.getBundlePath();
	const auto executablePath = inBundler.getExecutablePath();
	const auto frameworksPath = inBundler.getFrameworksPath();
	const auto resourcePath = inBundler.getResourcePath();

	makeBundlePath(bundlePath, executablePath, frameworksPath, resourcePath);

	// Timer timer;

	const auto cwd = m_inputs.workingDirectory() + '/';

	const auto copyIncludedPath = [&cwd](const std::string& inDep, const std::string& inOutPath) -> bool {
		if (Commands::pathExists(inDep))
		{
			const auto filename = String::getPathFilename(inDep);
			if (!filename.empty())
			{
				auto outputFile = fmt::format("{}/{}", inOutPath, filename);
				if (Commands::pathExists(outputFile))
					return true; // Already copied - duplicate dependency
			}

			auto dep = inDep;
			String::replaceAll(dep, cwd, "");

			if (!Commands::copy(dep, inOutPath))
			{
				Diagnostic::warn("Dependency '{}' could not be copied to: {}", filename, inOutPath);
				return false;
			}
		}
		return true;
	};

	for (auto& dep : bundle.includes())
	{
#if defined(CHALET_MACOS)
		if (String::endsWith(".framework", dep))
			continue;

		if (String::endsWith(".dylib", dep))
		{
			if (!copyIncludedPath(dep, frameworksPath))
				return false;

			// auto filename = String::getPathFilename(dep);
			// auto dylib = fmt::format("{}/{}", executablePath);
		}
		else
#endif
		{
			if (!copyIncludedPath(dep, resourcePath))
				return false;
		}
	}

	StringList executables;
	StringList dependenciesToCopy;
	StringList excludes;

	for (auto& target : inState.targets)
	{
		if (target->isSources())
		{
			auto& project = static_cast<const SourceTarget&>(*target);

			if (!List::contains(buildTargets, project.name()))
				continue;

			auto outputFilePath = inState.paths.getTargetFilename(project);
			if (project.isStaticLibrary())
			{
				List::addIfDoesNotExist(dependenciesToCopy, outputFilePath);
				continue;
			}
			else if (project.isExecutable())
			{
				std::string outTarget = outputFilePath;
				List::addIfDoesNotExist(executables, std::move(outTarget));
			}

			if (project.isSharedLibrary())
			{
				if (!copyIncludedPath(outputFilePath, frameworksPath))
					continue;
			}
			else
			{
				if (!copyIncludedPath(outputFilePath, executablePath))
					continue;
			}

			excludes.emplace_back(std::move(outputFilePath));
		}
	}

	m_dependencyMap.populateToList(dependenciesToCopy, excludes);

	std::sort(dependenciesToCopy.begin(), dependenciesToCopy.end());
	for (auto& dep : dependenciesToCopy)
	{
#if defined(CHALET_MACOS)
		if (String::endsWith(".framework", dep))
			continue;

		if (String::endsWith(".dylib", dep))
		{
			if (!copyIncludedPath(dep, frameworksPath))
				continue;
		}
		else
#endif
		{
			if (!copyIncludedPath(dep, executablePath))
				continue;
		}
	}

#if defined(CHALET_MACOS) || defined(CHALET_LINUX)
	for (auto& exec : executables)
	{
		const auto filename = String::getPathFilename(exec);
		const auto executable = fmt::format("{}/{}", executablePath, filename);

		if (!Commands::setExecutableFlag(executable))
		{
			Diagnostic::warn("Exececutable flag could not be set for: {}", executable);
			continue;
		}
	}
#endif

	// LOG("Distribution dependencies gathered in:", timer.asString());

	Commands::forEachGlobMatch(resourcePath, bundle.excludes(), GlobMatch::FilesAndFolders, [&](const fs::path& inPath) {
		Commands::remove(inPath.string());
	});

	if (!inBundler.bundleForPlatform())
		return false;

	return true;
}

/*****************************************************************************/
bool AppBundler::gatherDependencies(const BundleTarget& inTarget, BuildState& inState)
{
	if (!inTarget.includeDependentSharedLibraries())
		return true;

	const auto& buildTargets = inTarget.buildTargets();

	m_dependencyMap.addExcludesFromList(inTarget.includes());
	m_dependencyMap.clearSearchDirs();
	for (auto& target : inState.targets)
	{
		if (target->isSources())
		{
			auto& project = static_cast<const SourceTarget&>(*target);
			m_dependencyMap.addSearchDirsFromList(project.libDirs());
		}
	}

	StringList allDependencies;

#if defined(CHALET_MACOS)
	for (auto& dep : inTarget.includes())
	{
		if (String::endsWith(".dylib", dep))
			List::addIfDoesNotExist(allDependencies, dep);
	}
#endif

	for (auto& target : inState.targets)
	{
		if (target->isSources())
		{
			auto& project = static_cast<const SourceTarget&>(*target);

			if (!List::contains(buildTargets, project.name()))
				continue;

			auto outputFilePath = inState.paths.getTargetFilename(project);
			if (project.isStaticLibrary())
				continue;

			List::addIfDoesNotExist(allDependencies, std::move(outputFilePath));
		}
	}

	int levels = 2;
	if (!m_dependencyMap.gatherFromList(allDependencies, levels))
		return false;

	// m_dependencyMap.log();

	return true;
}

/*****************************************************************************/
bool AppBundler::runScriptTarget(const ScriptDistTarget& inTarget)
{
	const auto& scripts = inTarget.scripts();
	if (scripts.empty())
		return false;

	displayHeader("Script", inTarget);

	ScriptRunner scriptRunner(m_inputs, m_prototype.tools);
	bool showExitCode = false;
	if (!scriptRunner.run(scripts, showExitCode))
	{
		Output::lineBreak();
		Output::msgBuildFail(); // TODO: Script failed
		Output::lineBreak();
		return false;
	}

	return true;
}

/*****************************************************************************/
bool AppBundler::runArchiveTarget(const BundleArchiveTarget& inTarget)
{
	const auto& includes = inTarget.includes();
	if (includes.empty())
		return false;

	Timer timer;

	auto baseName = inTarget.name();

	BuildState* state = getBuildState(m_prototype.anyConfiguration());
	if (state != nullptr)
	{
		String::replaceAll(baseName, "(triple)", state->info.targetArchitectureTriple());
		String::replaceAll(baseName, "(arch)", state->info.targetArchitectureString());
	}

	auto filename = fmt::format("{}.zip", baseName);

	displayHeader("Compressing", inTarget, filename);

	StringList resolvedIncludes;
	for (auto& include : inTarget.includes())
	{
		Commands::addPathToListWithGlob(fmt::format("{}/{}", m_inputs.distributionDirectory(), include), resolvedIncludes, GlobMatch::FilesAndFolders);
	}

	// m_dependencyMap.log();

	Diagnostic::infoEllipsis("Compressing files");

	ZipArchiver zipArchiver(m_prototype);
	if (!zipArchiver.archive(baseName, resolvedIncludes, m_inputs.distributionDirectory(), m_archives))
		return false;

	m_archives.emplace_back(fmt::format("{}/{}", m_inputs.distributionDirectory(), filename));

	Diagnostic::printDone(timer.asString());
	return true;
}

/*****************************************************************************/
bool AppBundler::runMacosDiskImageTarget(const MacosDiskImageTarget& inTarget)
{
	displayHeader("Disk Image", inTarget);

	MacosDiskImageCreator diskImageCreator(m_inputs, m_prototype);
	if (!diskImageCreator.make(inTarget))
		return false;

	return true;
}

/*****************************************************************************/
bool AppBundler::runWindowsNullsoftInstallerTarget(const WindowsNullsoftInstallerTarget& inTarget)
{
	displayHeader("Nullsoft Installer", inTarget);

	WindowsNullsoftInstallerRunner nsis(m_prototype);
	if (!nsis.compile(inTarget))
		return false;

	return true;
}

/*****************************************************************************/
void AppBundler::displayHeader(const std::string& inLabel, const IDistTarget& inTarget, const std::string& inName) const
{
	if (!inTarget.description().empty())
		Output::msgTargetDescription(inTarget.description(), Output::theme().header);
	else
		Output::msgTargetOfType(inLabel, !inName.empty() ? inName : inTarget.name(), Output::theme().header);

	Output::lineBreak();
}

/*****************************************************************************/
bool AppBundler::removeOldFiles(IAppBundler& inBundler)
{
	const auto& bundle = inBundler.bundle();
	const auto& subdirectory = bundle.subdirectory();

	if (!List::contains(m_removedDirs, subdirectory))
	{
		Commands::removeRecursively(subdirectory);
		m_removedDirs.push_back(subdirectory);
	}

	if (!inBundler.removeOldFiles())
		return false;

	return true;
}

/*****************************************************************************/
bool AppBundler::makeBundlePath(const std::string& inBundlePath, const std::string& inExecutablePath, const std::string& inFrameworksPath, const std::string& inResourcePath)
{
	StringList dirList{ inBundlePath };
	List::addIfDoesNotExist(dirList, inExecutablePath);
	List::addIfDoesNotExist(dirList, inFrameworksPath);
	List::addIfDoesNotExist(dirList, inResourcePath);

	// make prod dir
	for (auto& dir : dirList)
	{
		if (Commands::pathExists(dir))
			continue;

		if (!Commands::makeDirectory(dir))
			return false;
	}

	return true;
}
}
