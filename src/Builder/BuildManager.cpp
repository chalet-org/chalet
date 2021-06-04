/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Builder/BuildManager.hpp"

#include "Builder/BuildManager/CmakeBuilder.hpp"
#include "Builder/BuildManager/ProfilerRunner.hpp"
#include "Builder/BuildManager/ScriptRunner.hpp"
#include "Router/Route.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Output.hpp"

#include "Libraries/Format.hpp"
#include "Terminal/Environment.hpp"
#include "Terminal/Path.hpp"
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
	})
{
}

/*****************************************************************************/
bool BuildManager::run(const Route inRoute)
{
	m_removeCache.clear();
	m_cleanOutput = m_state.environment.cleanOutput();

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

	auto strategy = m_state.environment.strategy();
	if (!runCommand)
	{
		printBuildInformation();

		Output::lineBreak();

		m_strategy = ICompileStrategy::make(strategy, m_state);
		if (!m_strategy->initialize())
			return false;

		for (auto& target : m_state.targets)
		{
			if (target->isProject())
			{
				if (!cacheRecipe(static_cast<const ProjectTarget&>(*target), inRoute))
					return false;
			}
		}

		m_strategy->saveBuildFile();
	}

	bool multiTarget = m_state.targets.size() > 1;

	const ProjectTarget* runProject = nullptr;

	bool error = false;
	for (auto& target : m_state.targets)
	{
		if ((runCommand || inRoute == Route::BuildRun) && target->isProject())
		{
			auto project = static_cast<const ProjectTarget*>(target.get());
			if (m_runProjectName == project->name())
				runProject = project;
			else if (runCommand)
				continue;
		}

		if (target->isCMake())
		{
			if (!runCMakeTarget(static_cast<const CMakeTarget&>(*target)))
				return false;
		}
		else if (target->isScript())
		{
			Timer buildTimer;

			if (!runScriptTarget(static_cast<const ScriptTarget&>(*target)))
			{
				error = true;
				break;
			}

			Output::print(Color::Reset, fmt::format("   Time: {}", buildTimer.asString()));
			Output::lineBreak();
		}
		else
		{
			Timer buildTimer;

			if (!m_buildRoutes[inRoute](*this, static_cast<const ProjectTarget&>(*target)))
			{
				error = true;
				break;
			}

			if (!runCommand)
			{
				Output::msgTargetUpToDate(multiTarget, target->name());
				Output::print(Color::Reset, fmt::format("   Time: {}", buildTimer.asString()));
				Output::lineBreak();
			}
		}
	}

	if (error)
		return false;

	if ((inRoute == Route::BuildRun || runCommand) && runProject != nullptr)
	{
		return doRun(*runProject);
	}
	else if (inRoute == Route::Build || inRoute == Route::Rebuild)
	{
		Output::msgBuildSuccess();
		Output::lineBreak();
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

	if (usingCpp && !m_state.compilerTools.compilerVersionStringCpp().empty())
	{
		Diagnostic::info(fmt::format("C++ Compiler: {}", m_state.compilerTools.compilerVersionStringCpp()));
	}
	if (usingCc && !m_state.compilerTools.compilerVersionStringC().empty())
	{
		Diagnostic::info(fmt::format("C Compiler: {}", m_state.compilerTools.compilerVersionStringC()));
	}

	const auto strategy = getBuildStrategyName();
	Diagnostic::info(fmt::format("Build Strategy: {}", strategy));
}

/*****************************************************************************/
std::string BuildManager::getBuildStrategyName() const
{
	std::string ret;

	switch (m_state.environment.strategy())
	{
		case StrategyType::Native:
			ret = "Native";
			break;

		case StrategyType::Ninja:
			ret = "Ninja";
			break;

		case StrategyType::Makefile: {
			m_state.tools.fetchMakeVersion();
			if (m_state.tools.makeIsNMake())
			{
				if (m_state.tools.makeIsJom())
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
bool BuildManager::cacheRecipe(const ProjectTarget& inProject, const Route inRoute)
{
	auto& compilerConfig = m_state.compilerTools.getConfig(inProject.language());
	auto compilerType = compilerConfig.compilerType();

	auto buildToolchain = ICompileToolchain::make(compilerType, m_state, inProject, compilerConfig);

	const bool objExtension = compilerType == CppCompilerType::VisualStudio;
	auto outputs = m_state.paths.getOutputs(inProject, compilerConfig.isMsvc(), m_state.environment.dumpAssembly(), objExtension);

	if (!Commands::makeDirectories(outputs.directories, m_cleanOutput))
	{
		Diagnostic::error(fmt::format("Error creating paths for project: {}", inProject.name()));
		return false;
	}

	if (!buildToolchain->initialize())
	{
		Diagnostic::error(fmt::format("Error preparing the build for project: {}", inProject.name()));
		return false;
	}

	if (inRoute == Route::Rebuild)
	{
		doClean(inProject, outputs.target, outputs.objectList, outputs.dependencyList);
	}

	return m_strategy->addProject(inProject, std::move(outputs), buildToolchain);
}

/*****************************************************************************/
bool BuildManager::copyRunDependencies(const ProjectTarget& inProject)
{
	bool result = true;

	const auto& workingDirectory = m_state.paths.workingDirectory();
	const auto& buildOutputDir = m_state.paths.buildOutputDir();
	auto& compilerConfig = m_state.compilerTools.getConfig(inProject.language());
	auto runDependencies = getResolvedRunDependenciesList(inProject.runDependencies(), compilerConfig);

	auto outputFolder = fmt::format("{}/{}", workingDirectory, buildOutputDir);

	int copied = 0;
	for (auto& dep : runDependencies)
	{
		auto depFile = String::getPathFilename(dep);
		if (!Commands::pathExists(fmt::format("{}/{}", outputFolder, depFile)))
		{
			result &= Commands::copy(dep, outputFolder, true);
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
			ret.push_back(std::move(resolved));
			continue;
		}

		for (auto& path : m_state.environment.path())
		{
			resolved = fmt::format("{}/{}", path, dep);
			if (Commands::pathExists(resolved))
			{
				ret.push_back(std::move(resolved));
				break;
			}
		}
	}

	return ret;
}

/*****************************************************************************/
bool BuildManager::doRun(const ProjectTarget& inProject)
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
				Diagnostic::error(fmt::format("There was an error copying run dependencies for: {}", project.name()));
				return false;
			}
		}
	}

	Output::msgBuildSuccess();

	auto outputFile = inProject.outputFile();
	if (outputFile.empty())
		return false;

	const auto& workingDirectory = m_state.paths.workingDirectory();
	const auto& buildOutputDir = m_state.paths.buildOutputDir();
	const auto& runOptions = m_inputs.runOptions();

	const auto& runArguments = inProject.runArguments();

	// LOG(workingDirectory);

	auto outputFolder = fmt::format("{}/{}", workingDirectory, buildOutputDir);

	Output::msgLaunch(buildOutputDir, outputFile);
	Output::lineBreak();

	// LOG(runOptions);
	// LOG(runArguments);

	auto file = fmt::format("{}/{}", outputFolder, outputFile);

	// LOG(file);

	if (!Commands::pathExists(file))
		return false;

	const auto& args = !runOptions.empty() ? runOptions : runArguments;

	StringList cmd = { file };
	for (auto& arg : args)
	{
		cmd.push_back(arg);
	}

	if (!m_state.configuration.enableProfiling())
	{
		if (!Commands::subprocess(cmd, m_cleanOutput))
		{
			Diagnostic::error(fmt::format("{} exited with code: {}", file, Subprocess::getLastExitCode()));
			return false;
		}

		return true;
	}

	return runProfiler(inProject, cmd, file, m_state.paths.buildOutputDir());
}

/*****************************************************************************/
bool BuildManager::runProfiler(const ProjectTarget& inProject, const StringList& inCommand, const std::string& inExecutable, const std::string& inOutputFolder)
{
	ProfilerRunner profiler(m_state, inProject, m_cleanOutput);
	return profiler.run(inCommand, inExecutable, inOutputFolder);
}

/*****************************************************************************/
bool BuildManager::doLazyClean()
{
	const auto& buildOutputDir = m_state.paths.buildOutputDir();
	const auto& buildPath = m_state.paths.buildPath();

	const auto& inputBuild = m_inputs.buildConfiguration();
	// const auto& build = m_state.buildConfiguration();

	std::string dirToClean;
	if (inputBuild.empty())
		dirToClean = buildPath;
	else
		dirToClean = buildOutputDir;

	if (!Commands::pathExists(dirToClean))
	{
		Output::msgNothingToClean();
		Output::lineBreak();
		return true;
	}

	if (m_cleanOutput)
	{
		Output::msgCleaning();
		Output::lineBreak();
	}

	Commands::removeRecursively(dirToClean, m_cleanOutput);

	// TODO: Clean CMake projects
	// TODO: Flag to clean externalDependencies
	// TODO: Also clean cache files specific to build configuration

	if (!m_cleanOutput)
		Output::lineBreak();

	return true;
}

/*****************************************************************************/
bool BuildManager::doClean(const ProjectTarget& inProject, const std::string& inTarget, const StringList& inObjectList, const StringList& inDepList, const bool inFullClean)
{
	// const auto& buildOutputDir = m_state.paths.buildOutputDir();

	// This prints for each project (bad)... maybe redundant since "Rebuild" is already printed
	// if (m_cleanOutput && Commands::pathExists(buildOutputDir))
	// {
	// 	Output::msgCleaningRebuild();
	// 	Output::lineBreak();
	// }

	auto pch = m_state.paths.getPrecompiledHeader(inProject);

	auto cacheAndRemove = [=](const StringList& inList, StringList& outCache) -> void {
		for (auto& item : inList)
		{
			if (!inFullClean && (inProject.usesPch() && String::contains(pch, item)))
				continue;

			if (List::contains(outCache, item))
				continue;

			outCache.push_back(item);
			Commands::remove(item, m_cleanOutput);
		}
	};

	if (!List::contains(m_removeCache, inTarget))
	{
		m_removeCache.push_back(inTarget);
		Commands::remove(inTarget, m_cleanOutput);
	}

	cacheAndRemove(inObjectList, m_removeCache);
	cacheAndRemove(inDepList, m_removeCache);

	return true;
}

/*****************************************************************************/
bool BuildManager::runScriptTarget(const ScriptTarget& inScript)
{
	const auto& scripts = inScript.scripts();
	if (scripts.empty())
		return false;

	if (!inScript.description().empty())
		Output::msgScriptDescription(inScript.description());
	else
		Output::msgScript(inScript.name());

	Output::lineBreak();

	ScriptRunner scriptRunner(m_state.tools, m_inputs.buildFile(), m_cleanOutput);
	if (!scriptRunner.run(scripts))
	{
		Output::lineBreak();
		Output::msgBuildFail(); // TODO: Script failed
		Output::lineBreak();
		return false;
	}

	return true;
}

/*****************************************************************************/
bool BuildManager::cmdBuild(const ProjectTarget& inProject)
{
	const auto& buildConfiguration = m_state.info.buildConfiguration();
	const auto& command = m_inputs.command();
	const auto& outputFile = inProject.outputFile();

	if (inProject.name() == m_runProjectName && command == Route::BuildRun)
		Output::msgBuildAndRun(buildConfiguration, outputFile);
	else
		Output::msgBuild(buildConfiguration, outputFile);

	Output::lineBreak();

	if (!m_strategy->buildProject(inProject))
	{
		Output::lineBreak();
		Output::msgBuildFail();
		Output::lineBreak();
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
	{
		Output::msgBuildFail();
		Output::lineBreak();
		return false;
	}

	return true;
}

/*****************************************************************************/
bool BuildManager::cmdRun(const ProjectTarget& inProject)
{
	const auto& buildConfiguration = m_state.info.buildConfiguration();
	const auto& outputFile = inProject.outputFile();

	Output::msgRun(buildConfiguration, outputFile);
	Output::lineBreak();

	return true;
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
bool BuildManager::runCMakeTarget(const CMakeTarget& inTarget)
{
	Timer buildTimer;

	CmakeBuilder cmake(m_state, inTarget, m_cleanOutput);
	if (!cmake.run())
		return false;

	auto result = buildTimer.stop();

	Output::print(Color::Reset, fmt::format("   Build time: {}ms", result));
	Output::lineBreak();

	return true;
}

/*****************************************************************************/
std::string BuildManager::getRunProject()
{
	const auto& runProjectArg = m_inputs.runProject();
	if (!runProjectArg.empty())
		return runProjectArg;

	for (auto& target : m_state.targets)
	{
		if (target->isProject())
		{
			auto& project = static_cast<const ProjectTarget&>(*target);
			if (project.isExecutable() && project.runProject())
				return project.name(); // just get the top one
		}
	}

	return std::string();
}

/*****************************************************************************/
void BuildManager::testTerminalMessages()
{
	const auto& buildConfiguration = m_state.info.buildConfiguration();
	// const auto& distConfig = m_state.bundle.configuration();
	const auto& buildOutputDir = m_state.paths.buildOutputDir();

	const std::string name{ "cool-program.exe" };
	const std::string profAnalysis{ "profiler_analysis.stats" };

	Output::msgBuildSuccess();
	Output::msgLaunch(buildOutputDir, name);
	Output::msgBuildFail();
	// Output::msgBuildProdError(distConfig);
	Output::msgProfilerDone(profAnalysis);
	Output::msgBuildAndRun(buildConfiguration, name);
	Output::msgBuild(buildConfiguration, name);
	Output::msgRebuild(buildConfiguration, name);
	Output::msgRun(buildConfiguration, name);
	Output::msgBuildProd(buildConfiguration, name);
	Output::msgProfile(buildConfiguration, name);
}
}