/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Bundler/AppBundler.hpp"

#include "BuildEnvironment/IBuildEnvironment.hpp"
#include "Builder/BatchValidator.hpp"
#include "Builder/ScriptRunner.hpp"
#include "Bundler/AppBundlerMacOS.hpp"
#include "Bundler/BinaryDependency/BinaryDependencyMap.hpp"
#include "Bundler/FileArchiver.hpp"
#include "Bundler/IAppBundler.hpp"
#include "Bundler/MacosDiskImageCreator.hpp"
#include "Cache/SourceCache.hpp"
#include "Core/CommandLineInputs.hpp"
#include "Process/Environment.hpp"
#include "Process/Process.hpp"
#include "Process/SubProcessController.hpp"
#include "State/BuildInfo.hpp"
#include "State/BuildPaths.hpp"
#include "State/BuildState.hpp"
#include "State/CentralState.hpp"
#include "State/CompilerTools.hpp"
#include "State/Distribution/BundleArchiveTarget.hpp"
#include "State/Distribution/BundleTarget.hpp"
#include "State/Distribution/MacosDiskImageTarget.hpp"
#include "State/Distribution/ProcessDistTarget.hpp"
#include "State/Distribution/ScriptDistTarget.hpp"
#include "State/Distribution/ValidationDistTarget.hpp"
#include "State/Target/SourceTarget.hpp"
#include "System/Files.hpp"
#include "Terminal/Output.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"
#include "Utility/Timer.hpp"

namespace chalet
{
/*****************************************************************************/
AppBundler::AppBundler(BuildState& inState) :
	m_state(inState)
{
}

/*****************************************************************************/
AppBundler::~AppBundler() = default;

/*****************************************************************************/
bool AppBundler::run(const DistTarget& inTarget)
{
	if (!inTarget->isArchive() && !isTargetNameValid(*inTarget))
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
			{
				Output::msgTargetOfType("Bundle", fmt::format("{}.{}", bundle.name(), bundle.macosBundleExtension()), Output::theme().header);
			}
			else
#elif defined(CHALET_LINUX)
			if (bundle.hasLinuxDesktopEntry())
			{
				Output::msgTargetOfType("Bundle", fmt::format("{}.desktop", bundle.name()), Output::theme().header);
			}
			else
#endif
			{
				Output::msgTargetOfType("Bundle", bundle.name(), Output::theme().header);
			}
		}

		m_dependencyMap = std::make_unique<BinaryDependencyMap>(m_state);
		m_dependencyMap->setIncludeWinUCRT(bundle.windowsIncludeRuntimeDlls());
		auto bundler = IAppBundler::make(m_state, bundle, *m_dependencyMap);
		if (!bundler->initialize())
		{
			Diagnostic::error("There was an error initializing the bundler for: {}", inTarget->name());
			return false;
		}

		if (!removeOldFiles(*bundler))
		{
			Diagnostic::error("There was an error removing the previous distribution bundle for: {}", inTarget->name());
			return false;
		}

		auto& distributionDirectory = m_state.inputs.distributionDirectory();
		if (!Files::pathExists(distributionDirectory))
			Files::makeDirectory(distributionDirectory);

#if defined(CHALET_MACOS)
		if (m_state.toolchain.strategy() == StrategyType::XcodeBuild && bundle.isMacosAppBundle())
		{
			if (!bundler->quickBundleForPlatform())
				return false;
		}
		else
#endif
		{
			if (!gatherDependencies(bundle))
				return false;

			if (!runBundleTarget(*bundler))
				return false;
		}
	}
	else
	{
		if (inTarget->isArchive())
		{
			if (!runArchiveTarget(static_cast<const BundleArchiveTarget&>(*inTarget)))
				return false;
		}
		else if (inTarget->isMacosDiskImage())
		{
			if (!runMacosDiskImageTarget(static_cast<const MacosDiskImageTarget&>(*inTarget)))
				return false;
		}
		else if (inTarget->isScript())
		{
			if (!runScriptTarget(static_cast<const ScriptDistTarget&>(*inTarget)))
				return false;
		}
		else if (inTarget->isProcess())
		{
			if (!runProcessTarget(static_cast<const ProcessDistTarget&>(*inTarget)))
				return false;
		}
		else if (inTarget->isValidation())
		{
			if (!runValidationTarget(static_cast<const ValidationDistTarget&>(*inTarget)))
				return false;
		}
	}

	Output::msgTargetUpToDate(inTarget->name(), &timer);
	Output::lineBreak();

	return true;
}

/*****************************************************************************/
void AppBundler::reportErrors()
{
	if (m_notCopied.empty())
		return;

	std::sort(m_notCopied.rbegin(), m_notCopied.rend());

	bool containsApiSet = false;
	bool validNotCopiedDeps = false;
	StringList excludes{ "msvcrt.dll", "kernel32.dll" };
	for (auto& dep : m_notCopied)
	{
		auto lower = String::toLowerCase(dep);
		if (String::equals(excludes, lower))
			continue;

		if (!containsApiSet && String::startsWith("api-ms-win-", lower))
			containsApiSet = true;

		Diagnostic::warn("{}", String::getPathFilename(dep));
		validNotCopiedDeps = true;
	}

	if (validNotCopiedDeps)
	{
		if (containsApiSet)
		{
			Diagnostic::warn("    https://learn.microsoft.com/en-us/windows/win32/apiindex/api-set-loader-operation");
			Diagnostic::warn("    https://learn.microsoft.com/en-us/windows/win32/apiindex/windows-apisets");
			Diagnostic::warn("  At least one of these may be a Windows API set:");
		}
		Diagnostic::warn("Dependencies not copied:");
	}
}

/*****************************************************************************/
bool AppBundler::runBundleTarget(IAppBundler& inBundler)
{
	auto& bundle = inBundler.bundle();
	auto buildTargets = bundle.getRequiredBuildTargets();
	const auto& bundleIncludes = bundle.includes();

	const auto bundlePath = inBundler.getBundlePath();
	const auto executablePath = inBundler.getExecutablePath();
	const auto frameworksPath = inBundler.getFrameworksPath();
	const auto resourcePath = inBundler.getResourcePath();

	makeBundlePath(bundlePath, executablePath, frameworksPath, resourcePath);

	// Timer timer;

#if defined(CHALET_MACOS)
	auto dylib = Files::getPlatformSharedLibraryExtension();
	auto framework = Files::getPlatformFrameworkExtension();
#endif

	for (auto& dep : bundleIncludes)
	{
#if defined(CHALET_MACOS)
		if (String::endsWith(framework, dep))
			continue;

		if (String::endsWith(dylib, dep))
		{
			if (!inBundler.copyIncludedPath(dep, frameworksPath))
				return false;

			// auto filename = String::getPathFilename(dep);
			// auto dylib = fmt::format("{}/{}", executablePath);
		}
		else
#endif
		{
			if (!inBundler.copyIncludedPath(dep, resourcePath))
				return false;
		}
	}

	StringList executables;
	StringList dependenciesToCopy;
	StringList excludes;

	for (auto& project : buildTargets)
	{
		auto outputFilePath = m_state.paths.getTargetFilename(*project);
		if (!project->copyFilesOnRun().empty())
		{
			StringList runDeps = project->getResolvedRunDependenciesList();
			for (auto& dep : runDeps)
			{
				List::addIfDoesNotExist(dependenciesToCopy, std::move(dep));
			}
		}
		if (project->isStaticLibrary())
		{
			List::addIfDoesNotExist(dependenciesToCopy, outputFilePath);
			continue;
		}
		else if (project->isExecutable())
		{
			std::string outTarget = outputFilePath;
			List::addIfDoesNotExist(executables, std::move(outTarget));
		}

		if (project->isSharedLibrary())
		{
			if (!inBundler.copyIncludedPath(outputFilePath, frameworksPath))
				continue;
		}
		else
		{
			if (!inBundler.copyIncludedPath(outputFilePath, executablePath))
				continue;
		}

		excludes.emplace_back(std::move(outputFilePath));
	}

	{
		auto& cwd = inBundler.workingDirectoryWithTrailingPathSeparator();
		m_dependencyMap->populateToList(dependenciesToCopy, excludes);
		for (auto& dep : dependenciesToCopy)
		{
			dep = Files::getCanonicalPath(dep);
			String::replaceAll(dep, cwd, "");
		}

		std::sort(dependenciesToCopy.begin(), dependenciesToCopy.end(), [this](const auto& inA, const auto& inB) {
			UNUSED(inB);
			return String::startsWith(m_state.paths.buildOutputDir(), inA);
		});

		for (auto& dep : dependenciesToCopy)
		{
#if defined(CHALET_MACOS)
			if (String::endsWith(framework, dep))
				continue;

			if (String::endsWith(dylib, dep))
			{
				if (!inBundler.copyIncludedPath(dep, frameworksPath))
					continue;
			}
			else
#endif
			{
				if (!inBundler.copyIncludedPath(dep, executablePath))
					continue;
			}
		}
	}

	if (!m_state.environment->isEmscripten())
	{
#if defined(CHALET_MACOS) || defined(CHALET_LINUX)
		for (auto& exec : executables)
		{
			const auto filename = String::getPathFilename(exec);
			const auto executable = fmt::format("{}/{}", executablePath, filename);

			if (!Files::setExecutableFlag(executable))
			{
				Diagnostic::warn("Executable flag could not be set for: {}", executable);
				continue;
			}
		}
#endif
	}

	// LOG("Distribution dependencies gathered in:", timer.asString());

	if (!Files::forEachGlobMatch(resourcePath, bundle.excludes(), GlobMatch::FilesAndFolders, [](const std::string& inPath) {
			Files::removeIfExists(inPath);
		}))
		return false;

	if (!inBundler.bundleForPlatform())
		return false;

	return true;
}

/*****************************************************************************/
bool AppBundler::gatherDependencies(BundleTarget& inTarget)
{
	if (!inTarget.resolveIncludes())
		return false;

	if (!inTarget.includeDependentSharedLibraries())
		return true;

	if (m_state.environment->isEmscripten())
		return true;

	auto buildTargets = inTarget.getRequiredBuildTargets();

	m_dependencyMap->clearSearchDirs();

	for (auto& target : m_state.targets)
	{
		if (target->isSources())
		{
			auto& project = static_cast<const SourceTarget&>(*target);

			const auto& compilerPathBin = m_state.toolchain.compilerCxx(project.language()).binDir;
			m_dependencyMap->addSearchDirsFromList(StringList{ compilerPathBin });
			m_dependencyMap->addSearchDirsFromList(project.libDirs());
		}
	}
	m_dependencyMap->addSearchDirsFromList(m_state.workspace.searchPaths());

	StringList allDependencies;

#if defined(CHALET_MACOS)
	auto dylib = Files::getPlatformSharedLibraryExtension();
#endif

#if defined(CHALET_MACOS)
	for (auto& dep : inTarget.includes())
	{
		if (String::endsWith(dylib, dep))
			List::addIfDoesNotExist(allDependencies, dep);
	}
#endif

	for (auto& project : buildTargets)
	{
		auto outputFilePath = m_state.paths.getTargetFilename(*project);
		if (project->isStaticLibrary())
			continue;

		List::addIfDoesNotExist(allDependencies, std::move(outputFilePath));
	}

	i32 levels = 2;
	if (!m_dependencyMap->gatherFromList(allDependencies, levels))
		return false;

	m_notCopied = List::combineRemoveDuplicates(m_notCopied, m_dependencyMap->notCopied());
	// m_dependencyMap->log();

	return true;
}

/*****************************************************************************/
bool AppBundler::runArchiveTarget(const BundleArchiveTarget& inTarget)
{
	const auto& includes = inTarget.includes();
	if (includes.empty())
		return false;

	auto baseName = inTarget.name();
	if (!isTargetNameValid(inTarget, baseName))
		return false;

	auto filename = inTarget.getOutputFilename(baseName);

	displayHeader("Compressing", inTarget, filename);

	Timer timer;

	Diagnostic::stepInfoEllipsis("Compressing files");

	FileArchiver archiver(m_state);
	if (!archiver.archive(inTarget, baseName, inTarget.includes(), m_archives))
		return false;

	m_archives.emplace_back(fmt::format("{}/{}", m_state.inputs.distributionDirectory(), filename));

	Diagnostic::printDone(timer.asString());

	if (!inTarget.macosNotarizationProfile().empty())
	{
		timer.restart();
		Diagnostic::stepInfoEllipsis("Notarizing archive");

		if (!archiver.notarize(inTarget))
			return false;

		Diagnostic::printDone(timer.asString());
	}

	return true;
}

/*****************************************************************************/
bool AppBundler::runMacosDiskImageTarget(const MacosDiskImageTarget& inTarget)
{
	if (m_state.environment->isEmscripten())
		return true;

	displayHeader("Disk Image", inTarget);

	MacosDiskImageCreator diskImageCreator(m_state);
	if (!diskImageCreator.make(inTarget))
		return false;

	return true;
}

/*****************************************************************************/
bool AppBundler::runScriptTarget(const ScriptDistTarget& inTarget)
{
	const auto& file = inTarget.file();
	if (file.empty())
		return false;

	displayHeader("Script", inTarget);

	const auto& arguments = inTarget.arguments();
	ScriptRunner scriptRunner(m_state.inputs, m_state.tools);
	bool showExitCode = false;

	if (scriptRunner.shouldRun(m_state.cache.file().sources(), StringList{}))
	{
		if (!scriptRunner.run(inTarget.scriptType(), file, arguments, showExitCode))
		{
			Diagnostic::printErrors(true);
			Output::previousLine();

			Output::lineBreak();
			Output::msgBuildFail();
			Output::lineBreak();
			return false;
		}
	}
	else
	{
		Output::msgTargetUpToDate(inTarget.name(), nullptr);
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
bool AppBundler::runValidationTarget(const ValidationDistTarget& inTarget)
{
	const auto& schema = inTarget.schema();
	if (schema.empty())
		return false;

	displayHeader("Validation", inTarget);

	BatchValidator validator(&m_state, inTarget.schema());
	bool result = validator.validate(inTarget.files(), false);
	// if (!result)
	// 	Output::lineBreak();

	return result;
}

/*****************************************************************************/
bool AppBundler::runProcess(const StringList& inCmd, std::string outputFile)
{
	bool result = Process::runWithInput(inCmd);

	m_state.inputs.clearWorkingDirectory(outputFile);

	i32 lastExitCode = SubProcessController::getLastExitCode();
	if (lastExitCode != 0)
	{
		auto message = fmt::format("{} exited with code: {}", outputFile, lastExitCode);
		Output::print(result ? Output::theme().info : Output::theme().error, message);
	}

	auto lastSystemMessage = SubProcessController::getSystemMessage(lastExitCode);
	if (!lastSystemMessage.empty())
	{
#if defined(CHALET_WIN32)
		String::replaceAll(lastSystemMessage, "%1", outputFile);
#endif
		Output::print(Output::theme().info, fmt::format("Error: {}", lastSystemMessage));
	}
	else if (lastExitCode < 0)
	{
		BinaryDependencyMap tmpMap(m_state);
		StringList dependencies;
		StringList dependenciesNotFound;

		tmpMap.setIncludeWinUCRT(true);
		if (tmpMap.getExecutableDependencies(outputFile, dependencies, &dependenciesNotFound) && !dependenciesNotFound.empty())
		{
			const auto& unknownDep = dependenciesNotFound.front();
			Output::print(Output::theme().info, fmt::format("Error: Cannot open shared object file: {}: No such file or directory.", unknownDep));
		}
	}

	return result;
}

/*****************************************************************************/
bool AppBundler::isTargetNameValid(const IDistTarget& inTarget) const
{
	if (String::contains(StringList{ "$", "{", "}" }, inTarget.name()))
	{
		Diagnostic::error("Variable(s) not allowed in target '{}' of its type.", inTarget.name());
		return false;
	}

	return true;
}

/*****************************************************************************/
bool AppBundler::isTargetNameValid(const IDistTarget& inTarget, std::string& outName) const
{
	auto buildFolder = String::getPathFolder(m_state.paths.buildOutputDir());
	String::replaceAll(outName, "${targetTriple}", m_state.info.targetArchitectureTriple());
	String::replaceAll(outName, "${toolchainName}", m_state.inputs.toolchainPreferenceName());
	String::replaceAll(outName, "${configuration}", m_state.configuration.name());
	String::replaceAll(outName, "${architecture}", m_state.info.targetArchitectureString());
	String::replaceAll(outName, "${buildDir}", buildFolder);

	if (String::contains(StringList{ "$", "{", "}" }, outName))
	{
		Diagnostic::error("Invalid variable(s) found in target '{}'", inTarget.name());
		return false;
	}

	return true;
}

/*****************************************************************************/
void AppBundler::displayHeader(const char* inLabel, const IDistTarget& inTarget, const std::string& inName) const
{
	auto& description = inTarget.outputDescription();
	if (!description.empty())
		Output::msgTargetDescription(description, Output::theme().header);
	else
		Output::msgTargetOfType(inLabel, !inName.empty() ? inName : inTarget.name(), Output::theme().header);

	// Output::lineBreak();
}

/*****************************************************************************/
bool AppBundler::removeOldFiles(IAppBundler& inBundler)
{
	const auto& bundle = inBundler.bundle();
	const auto& subdirectory = bundle.subdirectory();

	if (!List::contains(m_removedDirs, subdirectory))
	{
		Files::removeRecursively(subdirectory);
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
		if (Files::pathExists(dir))
			continue;

		if (!Files::makeDirectory(dir))
			return false;
	}

	return true;
}
}
