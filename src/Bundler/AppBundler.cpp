/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Bundler/AppBundler.hpp"

#include "Builder/BinaryDependencyMap.hpp"
#include "Builder/ScriptRunner.hpp"
#include "Bundler/FileArchiver.hpp"
#include "Bundler/IAppBundler.hpp"
#include "Bundler/MacosDiskImageCreator.hpp"
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

		m_dependencyMap = std::make_unique<BinaryDependencyMap>(m_state);
		auto bundler = IAppBundler::make(m_state, bundle, *m_dependencyMap, m_state.inputs.inputFile());
		if (!removeOldFiles(*bundler))
		{
			Diagnostic::error("There was an error removing the previous distribution bundle for: {}", inTarget->name());
			return false;
		}

		if (!bundle.resolveIncludesFromState(m_state))
			return false;

		if (!gatherDependencies(bundle))
			return false;

		if (!runBundleTarget(*bundler))
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
void AppBundler::reportErrors()
{
	if (m_notCopied.empty())
		return;

	std::sort(m_notCopied.rbegin(), m_notCopied.rend());

	bool validNotCopiedDeps = false;
	StringList excludes{ "msvcrt.dll", "kernel32.dll" };
	for (auto& dep : m_notCopied)
	{
		if (String::equals(excludes, String::toLowerCase(dep)))
			continue;

		Diagnostic::warn("{}", String::getPathFilename(dep));
		validNotCopiedDeps = true;
	}

	if (validNotCopiedDeps)
		Diagnostic::warn("Dependencies not copied:");
}

/*****************************************************************************/
bool AppBundler::runBundleTarget(IAppBundler& inBundler)
{
	auto& bundle = inBundler.bundle();
	const auto& buildTargets = inBundler.bundle().buildTargets();

	const auto bundlePath = inBundler.getBundlePath();
	const auto executablePath = inBundler.getExecutablePath();
	const auto frameworksPath = inBundler.getFrameworksPath();
	const auto resourcePath = inBundler.getResourcePath();

	makeBundlePath(bundlePath, executablePath, frameworksPath, resourcePath);

	// Timer timer;

	auto cwd = m_state.inputs.workingDirectory() + '/';
#if defined(CHALET_WIN32)
	cwd[0] = static_cast<char>(::toupper(static_cast<uchar>(cwd[0])));
#endif

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

	for (auto& target : m_state.targets)
	{
		if (target->isSources())
		{
			auto& project = static_cast<const SourceTarget&>(*target);

			if (!List::contains(buildTargets, project.name()))
				continue;

			if (!target->copyFilesOnRun().empty())
			{
				StringList runDeps = target->getResolvedRunDependenciesList();
				for (auto& dep : runDeps)
				{
					List::addIfDoesNotExist(dependenciesToCopy, std::move(dep));
				}
			}
			auto outputFilePath = m_state.paths.getTargetFilename(project);
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
	for (auto& dep : dependenciesToCopy)
	{
		String::replaceAll(dep, cwd, "");
	}

	std::sort(dependenciesToCopy.begin(), dependenciesToCopy.end(), [this](const auto& inA, const auto& inB) {
		UNUSED(inB);
		return String::startsWith(m_state.paths.buildOutputDir(), inA);
	});

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
bool AppBundler::gatherDependencies(const BundleTarget& inTarget)
{
	if (!inTarget.includeDependentSharedLibraries())
		return true;

	const auto& buildTargets = inTarget.buildTargets();

	m_dependencyMap->addExcludesFromList(inTarget.includes());
	m_dependencyMap->clearSearchDirs();
	for (auto& target : m_state.targets)
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

	for (auto& target : m_state.targets)
	{
		if (target->isSources())
		{
			auto& project = static_cast<const SourceTarget&>(*target);

			if (!List::contains(buildTargets, project.name()))
				continue;

			auto outputFilePath = m_state.paths.getTargetFilename(project);
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

	const auto& arguments = inTarget.arguments();
	ScriptRunner scriptRunner(m_state.inputs, m_state.tools);
	bool showExitCode = false;
	if (!scriptRunner.run(inTarget.scriptType(), file, arguments, showExitCode))
	{
		Diagnostic::printErrors(true);
		Output::previousLine();

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

	m_state.inputs.clearWorkingDirectory(outputFile);

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
		BinaryDependencyMap tmpMap(m_state);
		StringList dependencies;
		StringList dependenciesNotFound;

		if (tmpMap.getExecutableDependencies(outputFile, dependencies, &dependenciesNotFound) && !dependenciesNotFound.empty())
		{
			const auto& unknownDep = dependenciesNotFound.front();
			Output::print(Output::theme().info, fmt::format("Error: Cannot open shared object file: {}: No such file or directory.", unknownDep));
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

	return true;
}

/*****************************************************************************/
bool AppBundler::runMacosDiskImageTarget(const MacosDiskImageTarget& inTarget)
{
	displayHeader("Disk Image", inTarget);

	MacosDiskImageCreator diskImageCreator(m_state);
	if (!diskImageCreator.make(inTarget))
		return false;

	return true;
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
	String::replaceAll(outName, "${configuration}", m_state.info.buildConfiguration());
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
