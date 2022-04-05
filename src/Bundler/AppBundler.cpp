/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Bundler/AppBundler.hpp"

#include "Builder/BinaryDependencyMap.hpp"
#include "Builder/ScriptRunner.hpp"
#include "Bundler/IAppBundler.hpp"
#include "Bundler/MacosDiskImageCreator.hpp"
#include "Bundler/WindowsNullsoftInstallerRunner.hpp"
#include "Bundler/ZipArchiver.hpp"
#include "Compile/Environment/ICompileEnvironment.hpp"
#include "Core/CommandLineInputs.hpp"
#include "Process/ProcessController.hpp"
#include "State/BuildInfo.hpp"
#include "State/BuildPaths.hpp"
#include "State/BuildState.hpp"
#include "State/CentralState.hpp"
#include "State/Distribution/BundleArchiveTarget.hpp"
#include "State/Distribution/BundleTarget.hpp"
#include "State/Distribution/MacosDiskImageTarget.hpp"
#include "State/Distribution/ProcessDistTarget.hpp"
#include "State/Distribution/ScriptDistTarget.hpp"
#include "State/Distribution/WindowsNullsoftInstallerTarget.hpp"
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
AppBundler::AppBundler(CentralState& inCentralState) :
	m_centralState(inCentralState)
{
}

/*****************************************************************************/
AppBundler::~AppBundler() = default;

/*****************************************************************************/
bool AppBundler::runBuilds()
{
	// Build all required configurations
	m_detectedArch = m_centralState.inputs().targetArchitecture().empty() ? "auto" : m_centralState.inputs().targetArchitecture();

	auto makeState = [&](std::string arch, const std::string& inConfig) {
		auto configName = fmt::format("{}_{}", arch, inConfig);
		if (m_states.find(configName) == m_states.end())
		{
			CommandLineInputs inputs = m_centralState.inputs();
			inputs.setBuildConfiguration(std::string(inConfig));
			inputs.setTargetArchitecture(arch);
			auto state = std::make_unique<BuildState>(std::move(inputs), m_centralState);

			m_states.emplace(configName, std::move(state));
		}
	};

	StringList arches;
	for (auto& target : m_centralState.distribution)
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
	if (!inTarget->isArchive() && !isTargetNameValid(*inTarget))
		return false;

	if (!inTarget->initialize())
		return false;

	Timer timer;

	if (inTarget->isDistributionBundle())
	{
		auto& bundle = static_cast<BundleTarget&>(*inTarget);

		if (!bundle.outputDescription().empty())
		{
			Output::msgTargetDescription(bundle.outputDescription(), Output::theme().header);
		}
		else
		{
#if defined(CHALET_MACOS)
			if (bundle.isMacosAppBundle())
				Output::msgTargetOfType("Bundle", fmt::format("{}.{}", bundle.name(), bundle.macosBundleExtension()), Output::theme().header);
			else
#elif defined(CHALET_LINUX)
			if (bundle.hasLinuxDesktopEntry())
				Output::msgTargetOfType("Bundle", fmt::format("{}.desktop", bundle.name()), Output::theme().header);
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

		m_dependencyMap = std::make_unique<BinaryDependencyMap>(*buildState);
		auto bundler = IAppBundler::make(*buildState, bundle, *m_dependencyMap, m_centralState.inputs().inputFile());
		if (!removeOldFiles(*bundler))
		{
			Diagnostic::error("There was an error removing the previous distribution bundle for: {}", inTarget->name());
			return false;
		}

		if (!bundle.resolveIncludesFromState(*buildState))
			return false;

		if (!gatherDependencies(bundle, *buildState))
			return false;

		if (!runBundleTarget(*bundler, *buildState))
			return false;
	}
	else
	{
		if (inTarget->isScript())
		{
			if (!runScriptTarget(static_cast<const ScriptDistTarget&>(*inTarget)))
				return false;
		}
		else if (inTarget->isProcess())
		{
			if (!runProcessTarget(static_cast<const ProcessDistTarget&>(*inTarget)))
				return false;
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
#if defined(CHALET_LINUX)
			BuildState* state = getBuildState(m_centralState.anyConfiguration());
			if (state == nullptr)
			{
				Diagnostic::error("No associated build found for target: {}", inTarget->name());
				return false;
			}

			if (!state->environment->isMingw())
				return true;
#endif

			if (!runWindowsNullsoftInstallerTarget(static_cast<const WindowsNullsoftInstallerTarget&>(*inTarget)))
				return false;
		}
	}

	auto res = timer.stop();
	if (res > 0 && Output::showBenchmarks())
	{
		Output::printInfo(fmt::format("   Time: {}", timer.asString()));
	}
	Output::lineBreak();

	return true;
}

/*****************************************************************************/
void AppBundler::reportErrors() const
{
	StringList excludes{ "msvcrt.dll", "kernel32.dll" };
	for (auto& dep : m_notCopied)
	{
		if (String::equals(excludes, String::toLowerCase(dep)))
			continue;

		Diagnostic::warn("Dependency not copied because it was not found in {}: '{}'", Environment::getPathKey(), dep);
	}
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

	const auto cwd = m_centralState.inputs().workingDirectory() + '/';

	auto copyIncludedPath = [&cwd](const std::string& inDep, const std::string& inOutPath) -> bool {
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

	m_dependencyMap->populateToList(dependenciesToCopy, excludes);

	List::sort(dependenciesToCopy);
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

	if (!Commands::forEachGlobMatch(resourcePath, bundle.excludes(), GlobMatch::FilesAndFolders, [&](std::string inPath) {
			Commands::remove(inPath);
		}))
		return false;

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

	m_dependencyMap->addExcludesFromList(inTarget.includes());
	m_dependencyMap->clearSearchDirs();
	for (auto& target : inState.targets)
	{
		if (target->isSources())
		{
			auto& project = static_cast<const SourceTarget&>(*target);
			m_dependencyMap->addSearchDirsFromList(project.libDirs());
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
	if (!m_dependencyMap->gatherFromList(allDependencies, levels))
		return false;

	m_notCopied = List::combine(m_notCopied, m_dependencyMap->notCopied());
	// m_dependencyMap->log();

	return true;
}

/*****************************************************************************/
bool AppBundler::runScriptTarget(const ScriptDistTarget& inTarget)
{
	const auto& file = inTarget.file();
	if (file.empty())
		return false;

	displayHeader("Script", inTarget);

	ScriptRunner scriptRunner(m_centralState.inputs(), m_centralState.tools);
	bool showExitCode = false;
	if (!scriptRunner.run(file, showExitCode))
	{
		Output::lineBreak();
		Output::msgBuildFail();
		Output::lineBreak();
		return false;
	}

	return true;
}

/*****************************************************************************/
bool AppBundler::runProcessTarget(const ProcessDistTarget& inTarget)
{
	const auto& path = inTarget.path();
	if (path.empty())
		return false;

	displayHeader("Process", inTarget);

	StringList cmd;
	cmd.push_back(inTarget.path());
	for (auto& arg : inTarget.arguments())
	{
		cmd.push_back(arg);
	}

	bool result = runProcess(cmd, inTarget.path());

	if (!result)
	{
		Output::lineBreak();
		Output::msgBuildFail();
		Output::lineBreak();
		return false;
	}

	return true;
}

/*****************************************************************************/
bool AppBundler::runProcess(const StringList& inCmd, std::string outputFile)
{
	bool result = Commands::subprocessWithInput(inCmd);

	m_centralState.inputs().clearWorkingDirectory(outputFile);

	int lastExitCode = ProcessController::getLastExitCode();
	if (lastExitCode != 0)
	{
		auto message = fmt::format("{} exited with code: {}", outputFile, lastExitCode);
		Output::print(result ? Output::theme().info : Output::theme().error, message);
	}

	auto lastSystemMessage = ProcessController::getSystemMessage(lastExitCode);
	if (!lastSystemMessage.empty())
	{
#if defined(CHALET_WIN32)
		String::replaceAll(lastSystemMessage, "%1", outputFile);
#endif
		Output::print(Output::theme().info, fmt::format("Error: {}", lastSystemMessage));
	}
	else if (lastExitCode < 0)
	{
		auto state = getBuildState(m_centralState.anyConfiguration());
		if (state != nullptr)
		{
			BinaryDependencyMap tmpMap(*state);
			StringList dependencies;
			StringList dependenciesNotFound;

			if (tmpMap.getExecutableDependencies(outputFile, dependencies, &dependenciesNotFound) && !dependenciesNotFound.empty())
			{
				const auto& unknownDep = dependenciesNotFound.front();
				Output::print(Output::theme().info, fmt::format("Error: Cannot open shared object file: {}: No such file or directory.", unknownDep));
			}
		}
	}

	return result;
}

/*****************************************************************************/
bool AppBundler::runArchiveTarget(const BundleArchiveTarget& inTarget)
{
	const auto& includes = inTarget.includes();
	if (includes.empty())
		return false;

	auto baseName = inTarget.name();

	BuildState* state = getBuildState(m_centralState.anyConfiguration());
	if (state == nullptr)
	{
		Diagnostic::error("No associated build found for target: {}", inTarget.name());
		return false;
	}

	if (!isTargetNameValid(inTarget, *state, baseName))
		return false;

	auto filename = fmt::format("{}.zip", baseName);

	displayHeader("Compressing", inTarget, filename);

	StringList resolvedIncludes;
	for (auto& include : inTarget.includes())
	{
		Commands::addPathToListWithGlob(fmt::format("{}/{}", m_centralState.inputs().distributionDirectory(), include), resolvedIncludes, GlobMatch::FilesAndFolders);
	}

	Timer timer;

	Diagnostic::stepInfoEllipsis("Compressing files");

	ZipArchiver zipArchiver(m_centralState);
	if (!zipArchiver.archive(baseName, resolvedIncludes, m_centralState.inputs().distributionDirectory(), m_archives))
		return false;

	m_archives.emplace_back(fmt::format("{}/{}", m_centralState.inputs().distributionDirectory(), filename));

	Diagnostic::printDone(timer.asString());

	return true;
}

/*****************************************************************************/
bool AppBundler::runMacosDiskImageTarget(const MacosDiskImageTarget& inTarget)
{
	displayHeader("Disk Image", inTarget);

	MacosDiskImageCreator diskImageCreator(m_centralState);
	if (!diskImageCreator.make(inTarget))
		return false;

	return true;
}

/*****************************************************************************/
bool AppBundler::runWindowsNullsoftInstallerTarget(const WindowsNullsoftInstallerTarget& inTarget)
{
	displayHeader("Nullsoft Installer", inTarget);

	Diagnostic::info("Creating the Windows installer executable");

	WindowsNullsoftInstallerRunner nsis(m_centralState);
	if (!nsis.compile(inTarget))
		return false;

	return true;
}

/*****************************************************************************/
bool AppBundler::isTargetNameValid(const IDistTarget& inTarget) const
{
	if (String::contains({ "$", "{", "}" }, inTarget.name()))
	{
		Diagnostic::error("Variable(s) not allowed in target '{}' of its type.", inTarget.name());
		return false;
	}

	return true;
}

/*****************************************************************************/
bool AppBundler::isTargetNameValid(const IDistTarget& inTarget, const BuildState& inState, std::string& outName) const
{
	auto buildFolder = String::getPathFolder(inState.paths.buildOutputDir());
	String::replaceAll(outName, "${targetTriple}", inState.info.targetArchitectureTriple());
	String::replaceAll(outName, "${toolchainName}", m_centralState.inputs().toolchainPreferenceName());
	String::replaceAll(outName, "${configuration}", inState.info.buildConfiguration());
	String::replaceAll(outName, "${architecture}", inState.info.targetArchitectureString());
	String::replaceAll(outName, "${buildDir}", buildFolder);

	if (String::contains({ "$", "{", "}" }, outName))
	{
		Diagnostic::error("Invalid variable(s) found in target '{}'", inTarget.name());
		return false;
	}

	return true;
}

/*****************************************************************************/
void AppBundler::displayHeader(const std::string& inLabel, const IDistTarget& inTarget, const std::string& inName) const
{
	if (!inTarget.outputDescription().empty())
		Output::msgTargetDescription(inTarget.outputDescription(), Output::theme().header);
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
