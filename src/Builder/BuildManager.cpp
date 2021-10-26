/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Builder/BuildManager.hpp"

#include "Builder/CmakeBuilder.hpp"
#include "Builder/ProfilerRunner.hpp"
#include "Builder/ScriptRunner.hpp"
#include "Builder/SubChaletBuilder.hpp"
#include "Compile/CompilerConfigController.hpp"
#include "Core/CommandLineInputs.hpp"
#include "Process/Process.hpp"
#include "Router/Route.hpp"
#include "State/AncillaryTools.hpp"
#include "State/BuildConfiguration.hpp"
#include "State/BuildInfo.hpp"
#include "State/BuildPaths.hpp"
#include "State/BuildState.hpp"
#include "State/CompilerTools.hpp"
#include "State/Target/CMakeTarget.hpp"
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
BuildManager::BuildManager(const CommandLineInputs& inInputs, BuildState& inState) :
	m_inputs(inInputs),
	m_state(inState),
	m_buildRoutes({
		{ Route::BuildRun, &BuildManager::cmdBuild },
		{ Route::Build, &BuildManager::cmdBuild },
		{ Route::Rebuild, &BuildManager::cmdRebuild },
		{ Route::Run, &BuildManager::cmdRun },
		{ Route::Bundle, &BuildManager::cmdBuild },
	}),
	m_asmDumper(inInputs, inState)
{
}

/*****************************************************************************/
bool BuildManager::run(const Route inRoute, const bool inShowSuccess)
{
	m_timer.restart();

	m_removeCache.clear();

	if (inRoute == Route::Clean)
	{
		Output::lineBreak();

		if (!cmdClean())
			return false;

		Output::msgBuildSuccess();
		Output::lineBreak();
		return true;
	}

	if (m_buildRoutes.find(inRoute) == m_buildRoutes.end())
	{
		Output::lineBreak();
		Diagnostic::error("Build command not recognized.");
		return false;
	}

	bool runCommand = inRoute == Route::Run;
	m_runTargetName = getRunTarget();

	auto strategy = m_state.toolchain.strategy();
	if (!runCommand)
	{
		printBuildInformation();

		Output::lineBreak();
	}

	// Note: We still have to initialize the build when the command is "run"
	m_strategy = ICompileStrategy::make(strategy, m_state);
	if (!m_strategy->initialize())
		return false;

	if (!runCommand)
	{
		if (!m_asmDumper.validate())
			return false;

		for (auto& target : m_state.targets)
		{
			if (target->isProject())
			{
				if (!addProjectToBuild(static_cast<const SourceTarget&>(*target), inRoute))
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

	m_strategy->saveBuildFile();

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
		auto& name = target->name();
		if (runCommand || inRoute == Route::BuildRun)
		{
			if (m_runTargetName == name)
			{
				if (target->isProject())
				{
					auto& project = static_cast<const SourceTarget&>(*target);
					if (project.isExecutable())
						runTarget = target.get();
				}
				else if (target->isScript())
				{
					runTarget = target.get();
					continue;
				}
			}
			else if (m_runTargetName.empty() && runTarget == nullptr)
			{
				if (target->isProject())
				{
					auto& project = static_cast<const SourceTarget&>(*target);
					if (project.isExecutable())
						runTarget = target.get();
				}
				else if (target->isScript())
				{
					runTarget = target.get();
					continue;
				}
			}
			else if (runCommand)
				continue;
		}

		if (target->isSubChalet())
		{
			if (!runSubChaletTarget(static_cast<const SubChaletTarget&>(*target)))
				return false;
		}
		else if (target->isCMake())
		{
			if (!runCMakeTarget(static_cast<const CMakeTarget&>(*target)))
				return false;
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
		else
		{
			if (!runCommand)
			{
				Timer buildTimer;

				if (!m_buildRoutes[inRoute](*this, static_cast<const SourceTarget&>(*target)))
				{
					error = true;
					break;
				}

				Output::msgTargetUpToDate(multiTarget, name);
				auto res = buildTimer.stop();
				if (res > 0 && Output::showBenchmarks())
				{
					Output::printInfo(fmt::format("   Time: {}", buildTimer.asString()));
				}

				Output::lineBreak();
			}
		}
	}

	if (error)
	{
		if (!runCommand)
		{
			Output::msgBuildFail(); // TODO: Script failed
			Output::lineBreak();
		}
		return false;
	}
	else
	{
		if (!runCommand && inShowSuccess)
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
	}

	if ((inRoute == Route::BuildRun || runCommand))
	{
		if (runTarget == nullptr)
		{
			Diagnostic::error("No executable project was found to run.");
			return false;
		}
		else if (runTarget->isProject())
		{
			auto& project = static_cast<const SourceTarget&>(*runTarget);
			if (!Commands::pathExists(m_state.paths.getTargetFilename(project)))
			{
				Diagnostic::error("Requested configuration '{}' must be built for run target: '{}'", m_state.info.buildConfiguration(), project.name());
				return false;
			}

			// if (runCommand)
			Output::lineBreak();

			return cmdRun(project);
		}
		else if (runTarget->isScript())
		{
			auto& script = static_cast<const ScriptBuildTarget&>(*runTarget);

			// if (runCommand)
			Output::lineBreak();

			return runScriptTarget(script, true);
		}
		else
		{
			Diagnostic::error("Run target not found: '{}'", m_runTargetName);
			return false;
		}
	}

	return true;
}

/*****************************************************************************/
void BuildManager::printBuildInformation()
{
	bool usingCpp = false;
	bool usingCc = false;
	for (auto& target : m_state.targets)
	{
		if (target->isProject())
		{
			auto& project = static_cast<const SourceTarget&>(*target);

			usingCpp |= project.language() == CodeLanguage::CPlusPlus;
			usingCc |= project.language() == CodeLanguage::C;
		}
	}

	if (usingCpp && !m_state.toolchain.compilerDescriptionStringCpp().empty())
	{
		auto arch = m_state.toolchain.compilerDetectedArchCpp();
		if (!m_inputs.archOptions().empty())
		{
			arch += fmt::format(" ({})", String::join(m_inputs.archOptions(), ','));
		}
		Diagnostic::info("C++ Compiler: {}", m_state.toolchain.compilerDescriptionStringCpp());

		if (m_state.info.universalArches().empty())
			Diagnostic::info("Target Architecture: {}", arch);
		else
			Diagnostic::info("Target Architecture: {} ({})", arch, String::join(m_state.info.universalArches(), " / "));
	}

	if (usingCc && !m_state.toolchain.compilerDescriptionStringC().empty())
	{
		auto arch = m_state.toolchain.compilerDetectedArchC();
		if (!m_inputs.archOptions().empty())
		{
			arch += fmt::format(" ({})", String::join(m_inputs.archOptions(), ','));
		}
		Diagnostic::info("C Compiler: {}", m_state.toolchain.compilerDescriptionStringC());

		if (m_state.info.universalArches().empty())
			Diagnostic::info("Target Architecture: {}", arch);
		else
			Diagnostic::info("Target Architecture: {} ({})", arch, String::join(m_state.info.universalArches(), " / "));
	}

	const auto strategy = getBuildStrategyName();
	Diagnostic::info("Strategy: {}", strategy);

	Diagnostic::info("Configuration: {}", m_state.info.buildConfiguration());
}

/*****************************************************************************/
std::string BuildManager::getBuildStrategyName() const
{
	std::string ret;

	switch (m_state.toolchain.strategy())
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
bool BuildManager::addProjectToBuild(const SourceTarget& inProject, const Route inRoute)
{
	m_state.paths.setBuildDirectoriesBasedOnProjectKind(inProject);

	auto& compilerConfig = m_state.compilers.get(inProject.language());

	auto buildToolchain = ICompileToolchain::make(m_state.compilers.type(), m_state, inProject, compilerConfig);
	auto outputs = m_state.paths.getOutputs(inProject, m_state.info.dumpAssembly());

	if (!Commands::makeDirectories(outputs.directories, m_directoriesMade))
	{
		Diagnostic::error("Error creating paths for project: {}", inProject.name());
		return false;
	}

	if (!buildToolchain->initialize())
	{
		Diagnostic::error("Error preparing the build for project: {}", inProject.name());
		return false;
	}

	if (inRoute == Route::Rebuild)
	{
		bool fullClean = true;
		doClean(inProject, outputs.target, outputs.groups, fullClean);
	}

	if (!m_strategy->addProject(inProject, std::move(outputs), buildToolchain))
		return false;

	if (m_state.info.generateCompileCommands())
	{
		if (!m_strategy->addCompileCommands(inProject, buildToolchain))
			return false;
	}

	return true;
}

/*****************************************************************************/
bool BuildManager::copyRunDependencies(const SourceTarget& inProject)
{
	bool result = true;

	const auto& buildOutputDir = m_state.paths.buildOutputDir();
	auto& compilerConfig = m_state.compilers.get(inProject.language());
	auto runDependencies = getResolvedRunDependenciesList(inProject.runDependencies(), compilerConfig);

	auto outputFolder = Commands::getAbsolutePath(buildOutputDir);

	int copied = 0;
	for (auto& dep : runDependencies)
	{
		auto depFile = String::getPathFilename(dep);
		if (!Commands::pathExists(fmt::format("{}/{}", outputFolder, depFile)))
		{
			result &= Commands::copy(dep, outputFolder);
			copied++;
		}
	}

	if (copied > 0)
		Output::lineBreak();

	return result;
}

/*****************************************************************************/
StringList BuildManager::getResolvedRunDependenciesList(const StringList& inRunDependencies, const CompilerConfig& inConfig)
{
	StringList ret;
	const auto& compilerPathBin = inConfig.compilerPathBin();

	for (auto& dep : inRunDependencies)
	{
		if (Commands::pathExists(dep))
		{
			ret.push_back(dep);
			continue;
		}

		auto resolved = fmt::format("{}/{}", compilerPathBin, dep);
		if (Commands::pathExists(resolved))
		{
			ret.emplace_back(std::move(resolved));
			continue;
		}

		for (auto& path : m_state.workspace.searchPaths())
		{
			resolved = fmt::format("{}/{}", path, dep);
			if (Commands::pathExists(resolved))
			{
				ret.emplace_back(std::move(resolved));
				break;
			}
		}
	}

	return ret;
}

/*****************************************************************************/
bool BuildManager::runProfiler(const SourceTarget& inProject, const StringList& inCommand, const std::string& inExecutable, const std::string& inOutputFolder)
{
	ProfilerRunner profiler(m_inputs, m_state, inProject);
	return profiler.run(inCommand, inExecutable, inOutputFolder);
}

/*****************************************************************************/
bool BuildManager::doLazyClean()
{
	const auto& buildOutputDir = m_state.paths.buildOutputDir();
	const auto& outputDirectory = m_state.paths.outputDirectory();

	const auto& inputBuild = m_inputs.buildConfiguration();
	// const auto& build = m_state.buildConfiguration();

	std::string dirToClean;
	if (inputBuild.empty())
		dirToClean = outputDirectory;
	else
		dirToClean = buildOutputDir;

	if (!Commands::pathExists(dirToClean))
	{
		Output::msgNothingToClean();
		Output::lineBreak();
		return true;
	}

	if (Output::cleanOutput())
	{
		Output::msgCleaning();
		Output::lineBreak();
	}

	Commands::removeRecursively(dirToClean);

	// TODO: Clean CMake targets
	// TODO: Flag to clean externalDependencies
	// TODO: Also clean cache files specific to build configuration

	if (Output::showCommands())
		Output::lineBreak();

	return true;
}

/*****************************************************************************/
bool BuildManager::doClean(const SourceTarget& inProject, const std::string& inTarget, const SourceFileGroupList& inGroups, const bool inFullClean)
{
	auto pch = m_state.paths.getPrecompiledHeader(inProject);

	auto cacheAndRemove = [=](const std::string& inFile, StringList& outCache) -> void {
		if (inFile.empty())
			return;

		if (!inFullClean && (inProject.usesPch() && String::contains(pch, inFile)))
			return;

		if (List::contains(outCache, inFile))
			return;

		outCache.push_back(inFile);
		Commands::remove(inFile);
	};

	if (!List::contains(m_removeCache, inTarget))
	{
		m_removeCache.push_back(inTarget);
		Commands::remove(inTarget);
	}

	for (auto& group : inGroups)
	{
		cacheAndRemove(group->objectFile, m_removeCache);
		cacheAndRemove(group->dependencyFile, m_removeCache);
	}

	return true;
}

/*****************************************************************************/
bool BuildManager::doSubChaletClean(const SubChaletTarget& inTarget)
{
	auto outputLocation = fmt::format("{}/{}", m_inputs.outputDirectory(), inTarget.name());
	Path::sanitize(outputLocation);

	if (Commands::pathExists(outputLocation))
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
	auto outputLocation = fmt::format("{}/{}", Commands::getAbsolutePath(buildOutputDir), inTarget.location());
	Path::sanitize(outputLocation);

	if (Commands::pathExists(outputLocation))
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
bool BuildManager::runScriptTarget(const ScriptBuildTarget& inScript, const bool inRunCommand)
{
	const auto& scripts = inScript.scripts();
	if (scripts.empty())
		return false;

	const bool isRun = m_inputs.route() == Route::Run || inRunCommand;
	const Color color = isRun ? Output::theme().success : Output::theme().header;

	if (!inScript.description().empty())
		Output::msgTargetDescription(inScript.description(), color);
	else
		Output::msgScript(inScript.name(), color);

	Output::lineBreak();

	ScriptRunner scriptRunner(m_inputs, m_state.tools, m_inputs.inputFile());
	if (!scriptRunner.run(scripts, inRunCommand))
		return false;

	return true;
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

	if (!m_strategy->buildProject(inProject))
		return false;

	if (m_state.info.dumpAssembly())
	{
		if (!m_asmDumper.dumpProject(inProject.name(), m_strategy->getSourceOutput(inProject.name())))
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

	if (!m_strategy->buildProject(inProject))
		return false;

	if (m_state.info.dumpAssembly())
	{
		bool forced = true;
		if (!m_asmDumper.dumpProject(inProject.name(), m_strategy->getSourceOutput(inProject.name()), forced))
			return false;
	}

	return true;
}

/*****************************************************************************/
bool BuildManager::cmdRun(const SourceTarget& inProject)
{
	for (auto& target : m_state.targets)
	{
		if (target->isProject())
		{
			auto& project = static_cast<const SourceTarget&>(*target);
			if (project.runDependencies().empty())
				continue;

			if (!copyRunDependencies(project))
			{
				Diagnostic::error("There was an error copying run dependencies for: {}", project.name());
				return false;
			}
		}
	}

	auto outputFile = inProject.outputFile();
	if (outputFile.empty())
		return false;

	const auto& buildOutputDir = m_state.paths.buildOutputDir();
	const auto& runOptions = m_inputs.runOptions();

	const auto& runArguments = inProject.runArguments();

	if (!inProject.description().empty())
		Output::msgTargetDescription(inProject.description(), Output::theme().success);
	else
		Output::msgRun(outputFile);

	// LOG(runOptions);
	// LOG(runArguments);
	// Output::lineBreak();
	Output::printSeparator();
	// Output::lineBreak();

	auto file = fmt::format("{}/{}", Commands::getAbsolutePath(buildOutputDir), outputFile);

	if (!Commands::pathExists(file))
	{
		Diagnostic::error("Couldn't find file: {}", file);
		return false;
	}

#if defined(CHALET_MACOS)
	// This is required for profiling
	auto& installNameTool = m_state.tools.installNameTool();
	for (auto p : m_state.workspace.searchPaths())
	{
		String::replaceAll(p, m_state.paths.buildOutputDir() + '/', "");
		Commands::subprocessNoOutput({ installNameTool, "-add_rpath", fmt::format("@executable_path/{}", p), file });
	}
#endif

	const auto& args = !runOptions.empty() ? runOptions : runArguments;

	StringList cmd = { file };
	for (auto& arg : args)
	{
		cmd.push_back(arg);
	}

	if (!m_state.configuration.enableProfiling())
	{
		bool result = Commands::subprocess(cmd);

		auto outFile = fmt::format("{}/{}", buildOutputDir, outputFile);
		m_inputs.clearWorkingDirectory(outFile);

		int lastExitCode = Process::getLastExitCode();
		auto message = fmt::format("{} exited with code: {}", outFile, lastExitCode);

		// Output::lineBreak();
		Output::printSeparator();
		Output::print(result ? Output::theme().info : Output::theme().error, message);

		auto lastSystemMessage = Process::getSystemMessage(lastExitCode);
		if (!lastSystemMessage.empty())
		{
#if defined(CHALET_WIN32)
			String::replaceAll(lastSystemMessage, "%1", outputFile);
#endif
			Output::print(Output::theme().info, fmt::format("Error: {}", lastSystemMessage));
		}

		return result;
	}

	return runProfiler(inProject, cmd, file, m_state.paths.buildOutputDir());
}

/*****************************************************************************/
bool BuildManager::cmdClean()
{
	const auto& inputBuild = m_inputs.buildConfiguration();
	const auto& buildConfiguration = m_state.info.buildConfiguration();

	Output::msgClean(inputBuild.empty() ? inputBuild : buildConfiguration);
	Output::lineBreak();

	if (!doLazyClean())
	{
		return false;
	}

	return true;
}

/*****************************************************************************/
bool BuildManager::runSubChaletTarget(const SubChaletTarget& inTarget)
{
	Timer buildTimer;

	SubChaletBuilder subChalet(m_state, inTarget, m_inputs);
	if (!subChalet.run())
		return false;

	auto result = buildTimer.stop();
	if (result > 0 && Output::showBenchmarks())
	{
		Output::printInfo(fmt::format("   Build time: {}ms", result));
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
		Output::printInfo(fmt::format("   Build time: {}ms", result));
	}

	Output::lineBreak();

	return true;
}

/*****************************************************************************/
std::string BuildManager::getRunTarget()
{
	// Note: validated in BuildJsonParser::validRunTargetRequestedFromInput()
	//  before BuildManager is run
	//
	const auto& inputRunTarget = m_inputs.runTarget();
	if (!inputRunTarget.empty())
		return inputRunTarget;

	for (auto& target : m_state.targets)
	{
		auto& name = target->name();
		if (target->isProject())
		{
			auto& project = static_cast<const SourceTarget&>(*target);
			if (project.isExecutable() && project.runTarget())
				return name; // just get the top one
		}
	}

	return std::string();
}

}