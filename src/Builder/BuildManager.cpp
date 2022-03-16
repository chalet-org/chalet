/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Builder/BuildManager.hpp"

#include "Builder/BinaryDependencyMap.hpp"
#include "Builder/CmakeBuilder.hpp"
#include "Builder/ProfilerRunner.hpp"
#include "Builder/ScriptRunner.hpp"
#include "Builder/SubChaletBuilder.hpp"
#include "Compile/AssemblyDumper.hpp"
#include "Compile/CompileToolchainController.hpp"
#include "Compile/Environment/ICompileEnvironment.hpp"
#include "Core/CommandLineInputs.hpp"
#include "Process/ProcessController.hpp"
#include "Router/Route.hpp"
#include "State/AncillaryTools.hpp"
#include "State/BuildConfiguration.hpp"
#include "State/BuildInfo.hpp"
#include "State/BuildPaths.hpp"
#include "State/BuildState.hpp"
#include "State/CompilerTools.hpp"
#include "State/Target/CMakeTarget.hpp"
#include "State/Target/ProcessBuildTarget.hpp"
#include "State/Target/ScriptBuildTarget.hpp"
#include "State/Target/SourceTarget.hpp"
#include "State/Target/SubChaletTarget.hpp"
#include "State/WorkspaceEnvironment.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Environment.hpp"
#include "Terminal/Output.hpp"
#include "Terminal/Path.hpp"
#include "Terminal/Unicode.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"
#include "Utility/Timer.hpp"

#include "Json/JsonComments.hpp"

namespace chalet
{
/*****************************************************************************/
BuildManager::BuildManager(BuildState& inState) :
	m_state(inState),
	m_buildRoutes({
		{ Route::BuildRun, &BuildManager::cmdBuild },
		{ Route::Build, &BuildManager::cmdBuild },
		{ Route::Rebuild, &BuildManager::cmdRebuild },
		{ Route::Run, &BuildManager::cmdRun },
		{ Route::Bundle, &BuildManager::cmdBuild },
	})
{
}

/*****************************************************************************/
BuildManager::~BuildManager() = default;

/*****************************************************************************/
bool BuildManager::run(const Route inRoute, const bool inShowSuccess)
{
	m_timer.restart();

	if (inRoute == Route::Clean)
	{
		Output::lineBreak();

		if (!cmdClean())
			return false;

		Output::msgBuildSuccess();
		Output::lineBreak();
		return true;
	}
	else if (inRoute == Route::Rebuild)
	{
		// Don't produce any output from this
		doLazyClean();
	}

	if (m_buildRoutes.find(inRoute) == m_buildRoutes.end())
	{
		Output::lineBreak();
		Diagnostic::error("Build command not recognized.");
		return false;
	}

	bool runRoute = inRoute == Route::Run;
	const auto& runTargetName = m_state.inputs.runTarget();

	if (!runRoute)
	{
		printBuildInformation();

		Output::lineBreak();
	}

	// Note: We still have to initialize the build when the command is "run"
	m_strategy = ICompileStrategy::make(m_state.toolchain.strategy(), m_state);
	if (!m_strategy->initialize())
		return false;

	if (!runRoute)
	{
		if (m_state.info.dumpAssembly())
		{
			m_asmDumper = std::make_unique<AssemblyDumper>(m_state);
			if (!m_asmDumper->validate())
				return false;
		}

		for (auto& target : m_state.targets)
		{
			if (target->isSources())
			{
				if (!addProjectToBuild(static_cast<const SourceTarget&>(*target)))
					return false;
			}
			else if (inRoute == Route::Rebuild)
			{
				if (target->isCMake())
				{
					if (!doCMakeClean(static_cast<const CMakeTarget&>(*target)))
						return false;
				}
				else if (target->isSubChalet())
				{
					if (!doSubChaletClean(static_cast<const SubChaletTarget&>(*target)))
						return false;
				}
			}
		}
	}

	m_strategy->doPreBuild();
	m_fileListCacheShared.clear();
	m_fileListCache.clear();

	if (inRoute == Route::Rebuild || m_directoriesMade)
	{
		if (Output::showCommands())
			Output::lineBreak();
	}

	bool multiTarget = m_state.targets.size() > 1;

	const IBuildTarget* runTarget = nullptr;

	bool error = false;
	for (auto& target : m_state.targets)
	{
		bool isRunTarget = String::equals(runTargetName, target->name());
		bool noExplicitRunTarget = runTargetName.empty() && runTarget == nullptr;

		if (isRunTarget || noExplicitRunTarget)
		{
			runTarget = target.get();
			if (target->isScript())
				continue;
		}

		if (runRoute)
			continue;

		// At this point, we build
		if (target->isSubChalet())
		{
			if (!runSubChaletTarget(static_cast<const SubChaletTarget&>(*target)))
			{
				error = true;
				break;
			}
		}
		else if (target->isCMake())
		{
			if (!runCMakeTarget(static_cast<const CMakeTarget&>(*target)))
			{
				error = true;
				break;
			}
		}
		else if (target->isScript())
		{
			Timer buildTimer;

			if (!runScriptTarget(static_cast<const ScriptBuildTarget&>(*target), false))
			{
				error = true;
				break;
			}

			auto res = buildTimer.stop();
			if (res > 0 && Output::showBenchmarks())
			{
				Output::printInfo(fmt::format("   Time: {}", buildTimer.asString()));
			}

			Output::lineBreak();
		}
		else if (target->isProcess())
		{
			Timer buildTimer;

			if (!runProcessTarget(static_cast<const ProcessBuildTarget&>(*target)))
			{
				error = true;
				break;
			}

			auto res = buildTimer.stop();
			if (res > 0 && Output::showBenchmarks())
			{
				Output::printInfo(fmt::format("   Time: {}", buildTimer.asString()));
			}

			Output::lineBreak();
		}
		else
		{
			Timer buildTimer;

			if (!m_buildRoutes[inRoute](*this, static_cast<const SourceTarget&>(*target)))
			{
				error = true;
				break;
			}

			Output::msgTargetUpToDate(multiTarget, target->name());
			auto res = buildTimer.stop();
			if (res > 0 && Output::showBenchmarks())
			{
				Output::printInfo(fmt::format("   Time: {}", buildTimer.asString()));
			}

			Output::lineBreak();
		}
	}

	if (error)
	{
		if (!runRoute)
		{
			Output::msgBuildFail();
			Output::lineBreak();
		}
		return false;
	}
	else if (!runRoute && inShowSuccess)
	{
		if (!m_strategy->doPostBuild())
		{
			Diagnostic::error("The post-build step encountered a problem.");
			return false;
		}

		if (m_state.info.generateCompileCommands())
		{
			if (!m_strategy->saveCompileCommands())
			{
				Diagnostic::error("The post-build step encountered a problem.");
				return false;
			}
		}

		Output::msgBuildSuccess();

		auto res = m_timer.stop();
		if (res > 0 && Output::showBenchmarks())
		{
			Output::printInfo(fmt::format("   Total: {}", m_timer.asString()));
		}

		if (inRoute != Route::BuildRun)
			Output::lineBreak();
	}

	m_state.makeLibraryPathVariables();

	if (m_state.inputs.isRunRoute())
	{
		if (runTarget == nullptr)
		{
			Diagnostic::error("No executable project was found to run.");
			return false;
		}
		else if (runTarget->isSources() || runTarget->isCMake())
		{
			// if (runRoute)
			Output::lineBreak();

			return cmdRun(*runTarget);
		}
		else if (runTarget->isScript())
		{
			auto& script = static_cast<const ScriptBuildTarget&>(*runTarget);

			// if (runRoute)
			Output::lineBreak();

			return runScriptTarget(script, true);
		}
		else
		{
			Diagnostic::error("Run target not found: '{}'", runTargetName);
			return false;
		}
	}

	return true;
}

/*****************************************************************************/
void BuildManager::printBuildInformation()
{
	bool usingCpp = false;
	bool usingC = false;
	for (auto& target : m_state.targets)
	{
		if (target->isSources())
		{
			auto& project = static_cast<const SourceTarget&>(*target);

			usingCpp |= project.language() == CodeLanguage::CPlusPlus;
			usingC |= project.language() == CodeLanguage::C;
		}
	}

	auto printDetailsImpl = [](const CompilerInfo& inInfo, const std::string& inLang) -> void {
		if (inInfo.description.empty())
			return;

		Diagnostic::info("{} Compiler: {}", inLang, inInfo.description);
	};

	if (usingCpp)
		printDetailsImpl(m_state.toolchain.compilerCpp(), "C++");

	if (usingC)
		printDetailsImpl(m_state.toolchain.compilerC(), "C");

	const auto& hostArch = m_state.info.hostArchitectureString();
	if (hostArch != m_state.info.targetArchitectureString())
	{
		Diagnostic::info("Host Architecture: {}", hostArch);
	}

	auto arch = m_state.info.targetArchitectureTriple();
	if (!m_state.inputs.archOptions().empty())
	{
		arch += fmt::format(" ({})", String::join(m_state.inputs.archOptions(), ','));
	}
	if (m_state.inputs.universalArches().empty())
		Diagnostic::info("Target Architecture: {}", arch);
	else
		Diagnostic::info("Target Architecture: {} ({})", arch, String::join(m_state.inputs.universalArches(), " / "));

	const auto strategy = getBuildStrategyName();
	Diagnostic::info("Strategy: {}", strategy);
	Diagnostic::info("Configuration: {}", m_state.info.buildConfiguration());
}

/*****************************************************************************/
std::string BuildManager::getBuildStrategyName() const
{
	std::string ret;

	auto strategy = m_state.toolchain.strategy();
	switch (strategy)
	{
		case StrategyType::Native:
			ret = "Native";
			break;

		case StrategyType::Ninja:
			ret = "Ninja";
			break;

		case StrategyType::Makefile: {
			if (m_state.toolchain.makeIsNMake())
			{
				if (m_state.toolchain.makeIsJom())
					ret = "NMAKE (Qt Jom)";
				else
					ret = "NMAKE";
			}
			else
			{
				ret = "GNU Make";
			}
			break;
		}
		default: break;
	}

	return ret;
}

/*****************************************************************************/
bool BuildManager::addProjectToBuild(const SourceTarget& inProject)
{
	auto buildToolchain = std::make_unique<CompileToolchainController>(inProject);
	auto& fileCache = inProject.isSharedLibrary() ? m_fileListCacheShared : m_fileListCache;
	auto outputs = m_state.paths.getOutputs(inProject, fileCache, m_state.info.dumpAssembly());

	if (!Commands::makeDirectories(outputs->directories, m_directoriesMade))
	{
		Diagnostic::error("Error creating paths for project: {}", inProject.name());
		return false;
	}

	if (!buildToolchain->initialize(m_state))
	{
		Diagnostic::error("Error preparing the build for project: {}", inProject.name());
		return false;
	}

	m_strategy->setSourceOutputs(inProject, std::move(outputs));
	m_strategy->setToolchainController(inProject, std::move(buildToolchain));

	if (!inProject.cppModules())
	{
		if (!m_strategy->addProject(inProject))
			return false;
	}

	return true;
}

/*****************************************************************************/
bool BuildManager::copyRunDependencies(const IBuildTarget& inTarget, uint& outCopied)
{
	bool result = true;

	const auto& buildOutputDir = m_state.paths.buildOutputDir();
	auto copyFilesOnRun = getResolvedRunDependenciesList(inTarget);

	auto outputFolder = Commands::getAbsolutePath(buildOutputDir);

	for (auto& dep : copyFilesOnRun)
	{
		auto depFile = String::getPathFilename(dep);
		if (!Commands::pathExists(fmt::format("{}/{}", outputFolder, depFile)))
		{
			if (outCopied == 0)
				Output::lineBreak();

			result &= Commands::copy(dep, outputFolder);
			++outCopied;
		}
	}

	return result;
}

/*****************************************************************************/
StringList BuildManager::getResolvedRunDependenciesList(const IBuildTarget& inTarget)
{
	StringList ret;

	for (auto& dep : inTarget.copyFilesOnRun())
	{
		if (Commands::pathExists(dep))
		{
			ret.push_back(dep);
			continue;
		}

		std::string resolved;
		if (inTarget.isSources())
		{
			auto& project = static_cast<const SourceTarget&>(inTarget);
			const auto& compilerPathBin = m_state.toolchain.compilerCxx(project.language()).binDir;

			resolved = fmt::format("{}/{}", compilerPathBin, dep);
			if (Commands::pathExists(resolved))
			{
				ret.emplace_back(std::move(resolved));
				continue;
			}
		}

		bool found = false;
		for (auto& path : m_state.workspace.searchPaths())
		{
			resolved = fmt::format("{}/{}", path, dep);
			if (Commands::pathExists(resolved))
			{
				ret.emplace_back(std::move(resolved));
				found = true;
				break;
			}
		}

		if (!found)
		{
			resolved = Commands::which(dep);
			if (!resolved.empty())
			{
				ret.emplace_back(std::move(resolved));
			}
		}
	}

	return ret;
}

/*****************************************************************************/
bool BuildManager::runProfiler(const SourceTarget& inProject, const StringList& inCommand, const std::string& inExecutable)
{
	ProfilerRunner profiler(m_state, inProject);
	return profiler.run(inCommand, inExecutable);
}

/*****************************************************************************/
bool BuildManager::doLazyClean(const std::function<void()>& onClean)
{
	std::string buildOutputDir = m_state.paths.buildOutputDir();
	const auto& outputDirectory = m_state.paths.outputDirectory();

	const auto& inputBuild = m_state.inputs.buildConfiguration();
	// const auto& build = m_state.buildConfiguration();

	std::string dirToClean;
	if (inputBuild.empty())
		dirToClean = outputDirectory;
	else
		dirToClean = buildOutputDir;

	if (!Commands::pathExists(dirToClean))
		return false;

	StringList externalLocations;
	for (const auto& target : m_state.targets)
	{
		if (target->isSubChalet())
		{
			auto& subChaletTarget = static_cast<const SubChaletTarget&>(*target);
			List::addIfDoesNotExist(externalLocations, subChaletTarget.targetFolder());
		}
		else if (target->isCMake())
		{
			auto& cmakeTarget = static_cast<const CMakeTarget&>(*target);
			List::addIfDoesNotExist(externalLocations, cmakeTarget.targetFolder());
		}
	}

	CHALET_TRY
	{
		buildOutputDir += '/';
		for (const auto& entry : fs::recursive_directory_iterator(dirToClean))
		{
			auto path = entry.path().string();
			Path::sanitize(path);
			String::replaceAll(path, buildOutputDir, "");

			if (entry.is_regular_file())
			{
				if (String::startsWith(externalLocations, path))
					continue;

				fs::remove(entry.path());
			}
		}
	}
	CHALET_CATCH(...)
	{}

	auto buildDirs = m_state.paths.buildDirectories();
	for (auto& dir : buildDirs)
	{
		if (Commands::pathExists(dir))
			Commands::removeRecursively(dir);
	}

	if (Output::cleanOutput())
	{
		if (onClean != nullptr)
			onClean();
	}

	return true;
}

/*****************************************************************************/
bool BuildManager::doSubChaletClean(const SubChaletTarget& inTarget)
{
	auto outputLocation = fmt::format("{}/{}", m_state.inputs.outputDirectory(), inTarget.name());
	Path::sanitize(outputLocation);

	if (inTarget.rebuild() && Commands::pathExists(outputLocation))
	{
		if (!Commands::removeRecursively(outputLocation))
		{
			Diagnostic::error("There was an error cleaning the '{}' Chalet project.", inTarget.name());
			return false;
		}
	}

	return true;
}

/*****************************************************************************/
bool BuildManager::doCMakeClean(const CMakeTarget& inTarget)
{
	const auto& buildOutputDir = m_state.paths.buildOutputDir();
	auto outputLocation = fmt::format("{}/{}", Commands::getAbsolutePath(buildOutputDir), inTarget.targetFolder());
	Path::sanitize(outputLocation);

	if (inTarget.rebuild() && Commands::pathExists(outputLocation))
	{
		if (!Commands::removeRecursively(outputLocation))
		{
			Diagnostic::error("There was an error cleaning the '{}' CMake project.", inTarget.name());
			return false;
		}
	}

	return true;
}

/*****************************************************************************/
bool BuildManager::runScriptTarget(const ScriptBuildTarget& inTarget, const bool inRunCommand)
{
	const auto& file = inTarget.file();
	if (file.empty())
		return false;

	const Color color = inRunCommand ? Output::theme().success : Output::theme().header;

	if (!inTarget.description().empty())
		Output::msgTargetDescription(inTarget.description(), color);
	else
		Output::msgTargetOfType("Script", inTarget.name(), color);

	if (inRunCommand)
		Output::printSeparator();
	else
		Output::lineBreak();

	ScriptRunner scriptRunner(m_state.inputs, m_state.tools);
	if (!scriptRunner.run(file, inRunCommand))
		return false;

	return true;
}

/*****************************************************************************/
bool BuildManager::runProcessTarget(const ProcessBuildTarget& inTarget)
{
	const auto& path = inTarget.path();
	if (path.empty())
		return false;

	if (!inTarget.description().empty())
		Output::msgTargetDescription(inTarget.description(), Output::theme().header);
	else
		Output::msgTargetOfType("Process", inTarget.name(), Output::theme().header);

	Output::lineBreak();

	StringList cmd;
	cmd.push_back(inTarget.path());
	for (auto& arg : inTarget.arguments())
	{
		cmd.push_back(arg);
	}

	bool result = runProcess(cmd, inTarget.path(), false);

	if (!result)
		Output::lineBreak();

	return result;
}

/*****************************************************************************/
bool BuildManager::cmdBuild(const SourceTarget& inProject)
{
	const auto& outputFile = inProject.outputFile();

	if (!inProject.description().empty())
		Output::msgTargetDescription(inProject.description(), Output::theme().header);
	else
		Output::msgBuild(outputFile);

	Output::lineBreak();

	if (inProject.cppModules())
	{
		if (!m_strategy->buildProjectModules(inProject))
			return false;
	}
	else
	{
		if (!m_strategy->buildProject(inProject))
			return false;
	}

	if (m_state.info.dumpAssembly())
	{
		chalet_assert(m_asmDumper != nullptr, "");

		StringList fileCache;
		auto outputs = m_state.paths.getOutputs(inProject, fileCache, true);
		if (!m_asmDumper->dumpProject(inProject.name(), std::move(outputs)))
			return false;
	}

	return true;
}

/*****************************************************************************/
bool BuildManager::cmdRebuild(const SourceTarget& inProject)
{
	const auto& outputFile = inProject.outputFile();

	if (!inProject.description().empty())
		Output::msgTargetDescription(inProject.description(), Output::theme().header);
	else
		Output::msgRebuild(outputFile);

	Output::lineBreak();

	if (inProject.cppModules())
	{
		if (!m_strategy->buildProjectModules(inProject))
			return false;
	}
	else
	{
		if (!m_strategy->buildProject(inProject))
			return false;
	}

	if (m_state.info.dumpAssembly())
	{
		chalet_assert(m_asmDumper != nullptr, "");

		StringList fileCache;
		auto outputs = m_state.paths.getOutputs(inProject, fileCache, true);
		bool forced = true;
		if (!m_asmDumper->dumpProject(inProject.name(), std::move(outputs), forced))
			return false;
	}

	return true;
}

/*****************************************************************************/
bool BuildManager::cmdRun(const IBuildTarget& inTarget)
{
	const auto& buildOutputDir = m_state.paths.buildOutputDir();

	std::string outputFile;
	if (inTarget.isSources())
	{
		auto& project = static_cast<const SourceTarget&>(inTarget);
		outputFile = m_state.paths.getTargetFilename(project);
	}
	else if (inTarget.isCMake())
	{
		auto& project = static_cast<const CMakeTarget&>(inTarget);
		outputFile = project.runExecutable();

		if (!Commands::pathExists(outputFile))
		{
			outputFile = fmt::format("{}/{}", buildOutputDir, outputFile);
		}
#if defined(CHALET_WIN32)
		if (!Commands::pathExists(outputFile))
		{
			outputFile = fmt::format("{}.exe", outputFile);
		}
#else
		if (!Commands::pathExists(outputFile))
		{
			if (String::endsWith(".exe", outputFile))
				outputFile = outputFile.substr(0, outputFile.size() - 4);
		}
#endif
	}

	if (outputFile.empty() || !Commands::pathExists(outputFile))
	{
		Diagnostic::error("Requested configuration '{}' must be built for run target: '{}'", m_state.info.buildConfiguration(), inTarget.name());
		return false;
	}

	const auto& runArguments = m_state.inputs.runArguments();

	if (!inTarget.description().empty())
		Output::msgTargetDescription(inTarget.description(), Output::theme().success);
	else
		Output::msgRun(outputFile);

	auto file = Commands::getAbsolutePath(outputFile);
	if (!Commands::pathExists(file))
	{
		Diagnostic::error("Couldn't find file: {}", file);
		return false;
	}

	uint copied = 0;
	for (auto& target : m_state.targets)
	{
		if (!target->copyFilesOnRun().empty())
		{
			if (!copyRunDependencies(*target, copied))
			{
				Diagnostic::error("There was an error copying run dependencies for: {}", target->name());
				return false;
			}
		}

		if (String::equals(inTarget.name(), target->name()))
			break;
	}

	if (copied > 0)
		Output::lineBreak();

	StringList cmd = { file };
	if (runArguments.has_value())
	{
		for (auto& arg : *runArguments)
		{
			cmd.push_back(arg);
		}
	}

	if (inTarget.isSources() && m_state.configuration.enableProfiling())
	{
		Output::printSeparator();

		auto& project = static_cast<const SourceTarget&>(inTarget);
		return runProfiler(project, cmd, file);
	}
	else
	{
		return runProcess(cmd, outputFile, true);
	}
}

/*****************************************************************************/
bool BuildManager::runProcess(const StringList& inCmd, std::string outputFile, const bool inFromDist)
{
	if (inFromDist)
		Output::printSeparator();

	bool result = Commands::subprocessWithInput(inCmd);

	m_state.inputs.clearWorkingDirectory(outputFile);

	int lastExitCode = ProcessController::getLastExitCode();

	if (lastExitCode != 0 || inFromDist)
	{
		auto message = fmt::format("{} exited with code: {}", outputFile, lastExitCode);

		if (inFromDist)
			Output::printSeparator();

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
bool BuildManager::cmdClean()
{
	const auto& inputBuild = m_state.inputs.buildConfiguration();
	const auto& buildConfiguration = m_state.info.buildConfiguration();

	Output::msgClean(inputBuild.empty() ? inputBuild : buildConfiguration);
	Output::lineBreak();

	auto onClean = []() {
		Output::msgCleaning();
		Output::lineBreak();
	};

	if (!doLazyClean(onClean))
	{
		Output::msgNothingToClean();
		Output::lineBreak();
		return true;
	}

	if (Output::showCommands())
		Output::lineBreak();

	return true;
}

/*****************************************************************************/
bool BuildManager::runSubChaletTarget(const SubChaletTarget& inTarget)
{
	Timer buildTimer;

	SubChaletBuilder subChalet(m_state, inTarget);
	if (!subChalet.run())
		return false;

	auto result = buildTimer.stop();
	if (result > 0 && Output::showBenchmarks())
	{
		Output::printInfo(fmt::format("   Time: {}", buildTimer.asString()));
	}

	Output::lineBreak();

	return true;
}

/*****************************************************************************/
bool BuildManager::runCMakeTarget(const CMakeTarget& inTarget)
{
	Timer buildTimer;

	CmakeBuilder cmake(m_state, inTarget);
	if (!cmake.run())
		return false;

	auto result = buildTimer.stop();
	if (result > 0 && Output::showBenchmarks())
	{
		Output::printInfo(fmt::format("   Time: {}", buildTimer.asString()));
	}

	Output::lineBreak();

	return true;
}

}