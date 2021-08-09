/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Builder/BuildManager.hpp"

#include "Builder/CmakeBuilder.hpp"
#include "Builder/ProfilerRunner.hpp"
#include "Builder/ScriptRunner.hpp"
#include "Builder/SubChaletBuilder.hpp"
#include "Router/Route.hpp"
#include "State/AncillaryTools.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Output.hpp"

#include "State/Target/CMakeTarget.hpp"
#include "State/Target/ProjectTarget.hpp"
#include "State/Target/ScriptBuildTarget.hpp"
#include "State/Target/SubChaletTarget.hpp"

#include "Terminal/Environment.hpp"
#include "Terminal/Path.hpp"
#include "Terminal/Unicode.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"
#include "Utility/Subprocess.hpp"
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
		// { Route::kProfile, &BuildManager::cmdProfile },
		{ Route::Bundle, &BuildManager::cmdBuild },
	}),
	m_asmDumper(inState)
{
}

/*****************************************************************************/
bool BuildManager::run(const Route inRoute, const bool inShowSuccess)
{
#if defined(CHALET_MACOS)
	if (m_state.info.targetArchitecture() == Arch::Cpu::UniversalArm64_X64)
		return true;
#endif

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
	m_runProjectName = getRunProject();

	auto strategy = m_state.toolchain.strategy();
	if (!runCommand)
	{
		printBuildInformation();

		Output::lineBreak();
	}

	// Note: We still have to initialize the build when the command is "run"
	m_strategy = ICompileStrategy::make(strategy, m_state);
	if (!m_strategy->initialize(m_state.paths.allFileExtensions()))
		return false;

	if (!runCommand)
	{
		for (auto& target : m_state.targets)
		{
			if (target->isProject())
			{
				if (!addProjectToBuild(static_cast<const ProjectTarget&>(*target), inRoute))
					return false;
			}
		}
	}

	m_strategy->saveBuildFile();

	bool multiTarget = m_state.targets.size() > 1;

	const IBuildTarget* runProject = nullptr;

	bool error = false;
	for (auto& target : m_state.targets)
	{
		auto& name = target->name();
		if (runCommand || inRoute == Route::BuildRun)
		{
			if (m_runProjectName == name)
			{
				runProject = target.get();
				if (!target->isProject())
					continue;
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
			if (res > 0)
				Output::printInfo(fmt::format("   Time: {}", buildTimer.asString()));

			Output::lineBreak();
		}
		else
		{
			if (!runCommand)
			{
				Timer buildTimer;

				if (!m_buildRoutes[inRoute](*this, static_cast<const ProjectTarget&>(*target)))
				{
					error = true;
					break;
				}

				Output::msgTargetUpToDate(multiTarget, name);
				auto res = buildTimer.stop();
				if (res > 0)
					Output::printInfo(fmt::format("   Time: {}", buildTimer.asString()));

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

			if (m_state.tools.generateCompileCommands())
			{
				if (!m_strategy->saveCompileCommands())
				{
					Diagnostic::error("The post-build step encountered a problem.");
					return false;
				}
			}

			Output::msgBuildSuccess();
			Output::printInfo(fmt::format("   Total: {}", m_timer.asString()));

			if (inRoute != Route::BuildRun)
				Output::lineBreak();
		}
	}

	if ((inRoute == Route::BuildRun || runCommand) && runProject != nullptr)
	{
		if (runProject->isProject())
		{
			auto& project = static_cast<const ProjectTarget&>(*runProject);
			if (!Commands::pathExists(m_state.paths.getTargetFilename(project)))
			{
				Diagnostic::error("Requested configuration '{}' must be built for run project: '{}'", m_state.info.buildConfiguration(), project.name());
				return false;
			}

			// if (runCommand)
			Output::lineBreak();

			return cmdRun(project);
		}
		else if (runProject->isScript())
		{
			auto& script = static_cast<const ScriptBuildTarget&>(*runProject);

			// if (runCommand)
			Output::lineBreak();

			return runScriptTarget(script, true);
		}
		else
		{
			Diagnostic::error("Run project not found: '{}'", m_runProjectName);
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
			auto& project = static_cast<const ProjectTarget&>(*target);

			usingCpp |= project.language() == CodeLanguage::CPlusPlus;
			usingCc |= project.language() == CodeLanguage::C;
		}
	}

	if (usingCpp && !m_state.toolchain.compilerVersionStringCpp().empty())
	{
		auto arch = m_state.toolchain.compilerDetectedArchCpp();
		if (!m_inputs.archOptions().empty())
		{
			arch += fmt::format(" ({})", String::join(m_inputs.archOptions(), ','));
		}
		Diagnostic::info("C++ Compiler: {}", m_state.toolchain.compilerVersionStringCpp());
		Diagnostic::info("Target Architecture: {}", arch);
	}
	if (usingCc && !m_state.toolchain.compilerVersionStringC().empty())
	{
		auto arch = m_state.toolchain.compilerDetectedArchC();
		if (!m_inputs.archOptions().empty())
		{
			arch += fmt::format(" ({})", String::join(m_inputs.archOptions(), ','));
		}
		Diagnostic::info("C Compiler: {}", m_state.toolchain.compilerVersionStringC());
		Diagnostic::info("Target Architecture: {}", arch);
	}

	const auto strategy = getBuildStrategyName();
	Diagnostic::info("Build Strategy: {}", strategy);
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
bool BuildManager::addProjectToBuild(const ProjectTarget& inProject, const Route inRoute)
{
	auto& compilerConfig = m_state.toolchain.getConfig(inProject.language());
	auto compilerType = compilerConfig.compilerType();

	auto buildToolchain = ICompileToolchain::make(compilerType, m_state, inProject, compilerConfig);

	auto outputs = m_state.paths.getOutputs(inProject, compilerConfig, m_state.environment.dumpAssembly());

	if (!Commands::makeDirectories(outputs.directories))
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
		doClean(inProject, outputs.target, outputs.groups);
	}

	if (m_state.environment.dumpAssembly())
	{
		StringList assemblyList;
		for (auto& group : outputs.groups)
		{
			assemblyList.push_back(std::move(group->assemblyFile));
		}

		if (!m_asmDumper.addProject(inProject, std::move(assemblyList)))
			return false;
	}

	if (!m_strategy->addProject(inProject, std::move(outputs), buildToolchain))
		return false;

	if (m_state.tools.generateCompileCommands())
	{
		if (!m_strategy->addCompileCommands(inProject, buildToolchain))
			return false;
	}

	return true;
}

/*****************************************************************************/
bool BuildManager::copyRunDependencies(const ProjectTarget& inProject)
{
	bool result = true;

	const auto& buildOutputDir = m_state.paths.buildOutputDir();
	auto& compilerConfig = m_state.toolchain.getConfig(inProject.language());
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

		for (auto& path : m_state.environment.path())
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
bool BuildManager::runProfiler(const ProjectTarget& inProject, const StringList& inCommand, const std::string& inExecutable, const std::string& inOutputFolder)
{
	ProfilerRunner profiler(m_state, inProject);
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

	// TODO: Clean CMake projects
	// TODO: Flag to clean externalDependencies
	// TODO: Also clean cache files specific to build configuration

	if (Output::showCommands())
		Output::lineBreak();

	return true;
}

/*****************************************************************************/
bool BuildManager::doClean(const ProjectTarget& inProject, const std::string& inTarget, const SourceFileGroupList& inGroups, const bool inFullClean)
{
	// const auto& buildOutputDir = m_state.paths.buildOutputDir();

	// This prints for each project (bad)... maybe redundant since "Rebuild" is already printed
	// if (Output::cleanOutput() && Commands::pathExists(buildOutputDir))
	// {
	// 	Output::msgCleaningRebuild();
	// 	Output::lineBreak();
	// }

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
bool BuildManager::runScriptTarget(const ScriptBuildTarget& inScript, const bool inRunCommand)
{
	const auto& scripts = inScript.scripts();
	if (scripts.empty())
		return false;

	const bool isRun = m_inputs.command() == Route::Run || inRunCommand;
	const Color color = isRun ? Output::theme().success : Output::theme().header;

	if (!inScript.description().empty())
		Output::msgScriptDescription(inScript.description(), color);
	else
		Output::msgScript(inScript.name(), color);

	Output::lineBreak();

	ScriptRunner scriptRunner(m_state.tools, m_inputs.inputFile());
	if (!scriptRunner.run(scripts, inRunCommand))
		return false;

	return true;
}

/*****************************************************************************/
bool BuildManager::cmdBuild(const ProjectTarget& inProject)
{
	const auto& buildConfiguration = m_state.info.buildConfiguration();
	const auto& outputFile = inProject.outputFile();

	Output::msgBuild(buildConfiguration, outputFile);
	Output::lineBreak();

	if (!m_strategy->buildProject(inProject))
		return false;

	if (m_state.environment.dumpAssembly())
	{
		if (!m_asmDumper.dumpProject(inProject))
			return false;
	}

	return true;
}

/*****************************************************************************/
bool BuildManager::cmdRebuild(const ProjectTarget& inProject)
{
	const auto& buildConfiguration = m_state.info.buildConfiguration();
	const auto& outputFile = inProject.outputFile();

	Output::msgRebuild(buildConfiguration, outputFile);
	Output::lineBreak();

	if (!m_strategy->buildProject(inProject))
		return false;

	if (m_state.environment.dumpAssembly())
	{
		bool forced = true;
		if (!m_asmDumper.dumpProject(inProject, forced))
			return false;
	}

	return true;
}

/*****************************************************************************/
bool BuildManager::cmdRun(const ProjectTarget& inProject)
{
	for (auto& target : m_state.targets)
	{
		if (target->isProject())
		{
			auto& project = static_cast<const ProjectTarget&>(*target);
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
	const auto& buildConfiguration = m_state.info.buildConfiguration();

	const auto& runArguments = inProject.runArguments();

	Output::msgRun(buildConfiguration, outputFile);
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
	// install_name_tool -add_rpath @executable_path/chalet_external/SFML/lib
	for (auto p : m_state.environment.path())
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
		auto message = fmt::format("{} exited with code: {}", outFile, Subprocess::getLastExitCode());

		// Output::lineBreak();
		Output::printSeparator();
		Output::print(result ? Output::theme().info : Output::theme().error, message, true);

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
/*bool BuildManager::cmdProfile()
{
	chalet_assert(m_project != nullptr, "");

	const auto& buildConfiguration = m_state.buildConfiguration();
	const auto& outputFile = m_project->outputFile();

	Output::msgProfile(buildConfiguration, outputFile);
	Output::lineBreak();

	return true;
}*/

/*****************************************************************************/
bool BuildManager::runSubChaletTarget(const SubChaletTarget& inTarget)
{
	Timer buildTimer;

	SubChaletBuilder subChalet(m_state, inTarget, m_inputs);
	if (!subChalet.run())
		return false;

	auto result = buildTimer.stop();

	if (result > 0)
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

	if (result > 0)
	{
		Output::printInfo(fmt::format("   Build time: {}ms", result));
	}
	Output::lineBreak();

	return true;
}

/*****************************************************************************/
std::string BuildManager::getRunProject()
{
	// Note: validated in BuildJsonParser::validRunProjectRequestedFromInput()
	//  before BuildManager is run
	//
	const auto& inputRunProject = m_inputs.runProject();
	if (!inputRunProject.empty())
		return inputRunProject;

	for (auto& target : m_state.targets)
	{
		auto& name = target->name();
		if (target->isProject())
		{
			auto& project = static_cast<const ProjectTarget&>(*target);
			if (project.isExecutable() && project.runProject())
				return name; // just get the top one
		}
	}

	return std::string();
}

/*****************************************************************************/
void BuildManager::testTerminalMessages()
{
	const auto& buildConfiguration = m_state.info.buildConfiguration();
	// const auto& distConfig = m_state.bundle.configuration();

	const std::string name{ "cool-program.exe" };
	const std::string profAnalysis{ "profiler_analysis.stats" };

	Output::msgBuildSuccess();
	Output::msgBuildFail();
	// Output::msgBuildProdError(distConfig);
	Output::msgProfilerDone(profAnalysis);
	Output::msgBuild(buildConfiguration, name);
	Output::msgRebuild(buildConfiguration, name);
	Output::msgRun(buildConfiguration, name);
	Output::msgBuildProd(buildConfiguration, name);
	Output::msgProfile(buildConfiguration, name);
}
}