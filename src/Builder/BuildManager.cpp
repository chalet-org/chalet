/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Builder/BuildManager.hpp"

#include "Builder/BuildManager/CmakeBuilder.hpp"
#include "Builder/BuildManager/ProfilerRunner.hpp"
#include "Router/Route.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Output.hpp"

#include "Compile/CompileFactory.hpp"
#include "Libraries/Format.hpp"
#include "Terminal/Environment.hpp"
#include "Terminal/MsvcEnvironment.hpp"
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
		if (!cmdClean())
			return false;

		Output::msgBuildSuccess();
		Output::lineBreak();
		return true;
	}

	if (m_buildRoutes.find(inRoute) == m_buildRoutes.end())
	{
		Diagnostic::error("Build command not recognized.");
		return false;
	}

	bool runCommand = inRoute == Route::Run;
	m_runProjectName = getRunProject();

	bool error = false;
	for (auto& project : m_state.projects)
	{
		if (!project->includeInBuild())
			continue;

		if (runCommand && !project->runProject())
			continue;

		m_project = project.get();

		if (project->cmake())
		{
			// TODO: refactor this and compileCMakeProject()

			Timer buildTimer;

			if (compileCMakeProject())
			{
				if (runCommand)
					continue;

				auto result = buildTimer.stop();

				Output::print(Color::Reset, fmt::format("   Build time: {}ms", result));
				Output::lineBreak();
			}
			else
				return false;
		}
		else
		{
			Timer buildTimer;

			if (!m_buildRoutes[inRoute](*this))
			{
				error = true;
				break;
			}

			if (!project->scripts().empty())
			{
				auto result = buildTimer.stop();

				Output::print(Color::Reset, fmt::format("   Script time: {}ms", result));
				Output::lineBreak();
			}
			else if (!runCommand)
			{
				auto result = buildTimer.stop();

				Output::print(Color::Reset, fmt::format("   Build time: {}ms", result));
				Output::lineBreak();
			}
		}

		m_project = nullptr;
	}

	if (error)
		return false;

	if (inRoute == Route::BuildRun || inRoute == Route::Build || inRoute == Route::Rebuild)
	{
		Output::msgBuildSuccess();

		if (inRoute != Route::BuildRun)
			Output::lineBreak();
	}

	if (inRoute == Route::BuildRun || runCommand)
		return doRun();

	return true;
}

/*****************************************************************************/
bool BuildManager::doBuild(const Route inRoute)
{
	chalet_assert(m_project != nullptr, "");

	//
	auto& compilerConfig = m_state.compilers.getConfig(m_project->language());
	auto compilerType = compilerConfig.compilerType();
	const bool objExtension = compilerType == CppCompilerType::VisualStudio;
	auto outputs = m_state.paths.getOutputs(*m_project, objExtension);

	if (!Commands::makeDirectories(outputs.directories, m_cleanOutput))
	{
		Diagnostic::errorAbort(fmt::format("Error creating paths for project: {}", m_project->name()));
		return false;
	}

	if (Environment::isMsvc())
	{
		MsvcEnvironment msvcEnvironment;
		msvcEnvironment.readCompilerVariables();
	}

	if (inRoute == Route::Rebuild)
		doClean(outputs.objectList, outputs.dependencyList);

	{
		auto buildToolchain = CompileFactory::makeToolchain(compilerType, m_state, *m_project, compilerConfig);

		auto strategyType = m_state.environment.strategy();
		auto buildStrategy = CompileFactory::makeStrategy(strategyType, m_state, *m_project, buildToolchain);

		if (!buildStrategy->createCache(outputs))
		{
			Diagnostic::errorAbort(fmt::format("Project cache could not be created for: {}", m_project->name()));
			return false;
		}

		if (!buildStrategy->initialize())
			return false;

		if (!buildStrategy->run())
			return false;
	}

	if (inRoute == Route::BuildRun)
	{
		if (!copyRunDependencies())
		{
			Diagnostic::error(fmt::format("There was an error copying run dependencies for: {}", m_project->name()));
			return false;
		}
	}

	//
	bool multiTarget = m_state.projects.size() > 1;
	Output::msgTargetUpToDate(multiTarget, m_project->name());

	return true;
}

/*****************************************************************************/
bool BuildManager::doScript()
{
	chalet_assert(m_project != nullptr, "");

	const auto& scripts = m_project->scripts();
	if (scripts.empty())
		return false;

	if (!runExternalScripts(scripts))
	{
		Diagnostic::error(fmt::format("There was a problem running the script(s) for the build step: {}", m_project->name()));
		return false;
	}

	return true;
}

/*****************************************************************************/
bool BuildManager::copyRunDependencies()
{
	chalet_assert(m_project != nullptr, "");

	bool result = true;

	if (m_project->name() == m_runProjectName)
	{
		const auto& workingDirectory = m_state.paths.workingDirectory();
		const auto& buildOutputDir = m_state.paths.buildOutputDir();
		auto& compilerConfig = m_state.compilers.getConfig(m_project->language());
		auto runDependencies = getResolvedRunDependenciesList(compilerConfig);

		auto outputFolder = fmt::format("{workingDirectory}/{buildOutputDir}",
			FMT_ARG(workingDirectory),
			FMT_ARG(buildOutputDir));

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
	}

	return result;
}

/*****************************************************************************/
StringList BuildManager::getResolvedRunDependenciesList(const CompilerConfig& inConfig)
{
	chalet_assert(m_project != nullptr, "");

	StringList ret;
	const auto& compilerPathBin = inConfig.compilerPathBin();

	for (auto& dep : m_project->runDependencies())
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
bool BuildManager::doRun()
{
	auto outputFile = getRunOutputFile();
	if (outputFile.empty())
		return false;

	const auto& workingDirectory = m_state.paths.workingDirectory();
	const auto& buildOutputDir = m_state.paths.buildOutputDir();
	const auto& runOptions = m_inputs.runOptions();

	const auto& runArguments = m_project->runArguments();

	// LOG(workingDirectory);

	auto outputFolder = fmt::format("{workingDirectory}/{buildOutputDir}",
		FMT_ARG(workingDirectory),
		FMT_ARG(buildOutputDir));

	Output::msgLaunch(buildOutputDir, outputFile);
	Output::lineBreak();

	// LOG(runOptions);
	// LOG(runArguments);

	auto file = fmt::format("{outputFolder}/{outputFile}",
		FMT_ARG(outputFolder),
		FMT_ARG(outputFile));

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

	return runProfiler(cmd, file, m_state.paths.buildOutputDir());
}

/*****************************************************************************/
bool BuildManager::runProfiler(const StringList& inCommand, const std::string& inExecutable, const std::string& inOutputFolder)
{
	chalet_assert(m_project != nullptr, "");

	ProfilerRunner profiler(m_state, *m_project, m_cleanOutput);
	return profiler.run(inCommand, inExecutable, inOutputFolder);
}

/*****************************************************************************/
bool BuildManager::doLazyClean()
{
	const auto& buildOutputDir = m_state.paths.buildOutputDir();
	const auto& buildDir = m_state.paths.buildDir();

	const auto& inputBuild = m_inputs.buildConfiguration();
	// const auto& build = m_state.buildConfiguration();

	std::string dirToClean;
	if (inputBuild.empty())
		dirToClean = buildDir;
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
bool BuildManager::doClean(const StringList& inObjectList, const StringList& inDepList, const bool inFullClean)
{
	chalet_assert(m_project != nullptr, "");

	const auto& buildOutputDir = m_state.paths.buildOutputDir();

	if (m_cleanOutput && Commands::pathExists(buildOutputDir))
	{
		Output::msgCleaningRebuild();
		Output::lineBreak();
	}

	auto pch = m_state.paths.getPrecompiledHeader(*m_project);

	auto cacheAndRemove = [=](const StringList& inList, StringList& outCache) -> void {
		for (auto& item : inList)
		{
			if (!inFullClean && (m_project->usesPch() && String::contains(pch, item)))
				continue;

			if (List::contains(outCache, item))
				continue;

			outCache.push_back(item);
			Commands::remove(item, m_cleanOutput);
		}
	};

	std::string target = m_state.paths.getTargetFilename(*m_project);
	if (!List::contains(m_removeCache, target))
	{
		m_removeCache.push_back(target);
		Commands::remove(target, m_cleanOutput);
	}

	cacheAndRemove(inObjectList, m_removeCache);
	cacheAndRemove(inDepList, m_removeCache);

	return true;
}

/*****************************************************************************/
bool BuildManager::cmdBuild()
{
	chalet_assert(m_project != nullptr, "");

	const auto& buildConfiguration = m_state.buildConfiguration();
	const auto& command = m_inputs.command();
	const auto& outputFile = m_project->outputFile();
	const bool isScript = !m_project->scripts().empty();

	if (isScript)
	{
		if (!m_project->description().empty())
			Output::msgScriptDescription(m_project->description());
		else
			Output::msgScript(m_project->name());
	}
	else
	{
		if (m_project->name() == m_runProjectName && command == Route::BuildRun)
			Output::msgBuildAndRun(buildConfiguration, outputFile);
		else
			Output::msgBuild(buildConfiguration, outputFile);
	}

	Output::lineBreak();

	bool result = false;
	if (!m_project->scripts().empty())
	{
		result = doScript();
	}
	else
	{
		result = doBuild(command);
	}

	if (!result)
	{
		Output::msgBuildFail();
		Output::lineBreak();
		return false;
	}

	return true;
}

/*****************************************************************************/
bool BuildManager::cmdRebuild()
{
	chalet_assert(m_project != nullptr, "");

	const auto& buildConfiguration = m_state.buildConfiguration();
	const auto& outputFile = m_project->outputFile();

	Output::msgRebuild(buildConfiguration, outputFile);
	Output::lineBreak();

	if (!doBuild(Route::Rebuild))
	{
		Output::msgBuildFail();
		Output::lineBreak();
		return false;
	}

	return true;
}

/*****************************************************************************/
bool BuildManager::cmdRun()
{
	chalet_assert(m_project != nullptr, "");

	const auto& buildConfiguration = m_state.buildConfiguration();
	auto outputFile = getRunOutputFile();

	Output::msgRun(buildConfiguration, outputFile);
	Output::lineBreak();

	return true;
}

/*****************************************************************************/
bool BuildManager::cmdClean()
{
	const auto& inputBuild = m_inputs.buildConfiguration();
	const auto& buildConfiguration = m_state.buildConfiguration();

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
bool BuildManager::compileCMakeProject()
{
	chalet_assert(m_project != nullptr, "");
	chalet_assert(m_project->cmake(), "");

	CmakeBuilder cmake{ m_state, *m_project, m_cleanOutput };
	return cmake.run();
}

/*****************************************************************************/
std::string BuildManager::getRunProject()
{
	const auto& runProjectArg = m_inputs.runProject();
	if (!runProjectArg.empty())
		return runProjectArg;

	for (auto& project : m_state.projects)
	{
		if (project->isExecutable() && project->runProject())
			return project->name(); // just get the top one
	}

	return std::string();
}

/*****************************************************************************/
std::string BuildManager::getRunOutputFile()
{
	m_project = nullptr;

	std::string outputFile;
	for (auto& project : m_state.projects)
	{
		if (!project->includeInBuild())
			continue;

		if (project->name() == m_runProjectName)
		{
			outputFile = project->outputFile();
			m_project = project.get();

			// LOG(proj->runArguments());

			break;
		}
	}

	if (m_project == nullptr)
		m_project = m_state.projects.back().get();

	return outputFile;
}

/*****************************************************************************/
bool BuildManager::runExternalScripts(const StringList& inScripts)
{

	bool result = true;
	for (auto& scriptPath : inScripts)
	{
		std::ptrdiff_t i = &scriptPath - &inScripts.front();
		if (i == 0)
		{
			std::cout << Output::getAnsiReset() << std::flush;
		}

#if defined(CHALET_WIN32)
		std::string parsedScriptPath = scriptPath;
		if (String::endsWith(".exe", parsedScriptPath))
			parsedScriptPath = parsedScriptPath.substr(0, parsedScriptPath.size() - 4);

		auto outScriptPath = Commands::which(parsedScriptPath);
#else
		auto outScriptPath = Commands::which(scriptPath);
#endif
		if (outScriptPath.empty())
			outScriptPath = fs::absolute(scriptPath).string();

		if (!Commands::pathExists(outScriptPath))
		{
			Diagnostic::error(fmt::format("{}: The script '{}' was not found. Aborting.", CommandLineInputs::file(), scriptPath));
			return false;
		}

		Commands::setExecutableFlag(outScriptPath, m_cleanOutput);

		StringList command;

		const bool isBashScript = String::endsWith(".sh", outScriptPath);
		if (isBashScript && m_state.tools.bashAvailable())
		{
			command.push_back(m_state.tools.bash());
		}

		command.push_back(std::move(outScriptPath));

		result &= Commands::subprocess(command, m_cleanOutput);
	}

	return result;
}

/*****************************************************************************/
void BuildManager::testTerminalMessages()
{
	const auto& buildConfiguration = m_state.buildConfiguration();
	const auto& distConfig = m_state.bundle.configuration();
	const auto& buildOutputDir = m_state.paths.buildOutputDir();

	const std::string name{ "cool-program.exe" };
	const std::string profAnalysis{ "profiler_analysis.stats" };

	Output::msgBuildSuccess();
	Output::msgLaunch(buildOutputDir, name);
	Output::msgBuildFail();
	Output::msgBuildProdError(distConfig);
	Output::msgProfilerDone(profAnalysis);
	Output::msgBuildAndRun(buildConfiguration, name);
	Output::msgBuild(buildConfiguration, name);
	Output::msgRebuild(buildConfiguration, name);
	Output::msgRun(buildConfiguration, name);
	Output::msgBuildProd(buildConfiguration, name);
	Output::msgProfile(buildConfiguration, name);
}
}