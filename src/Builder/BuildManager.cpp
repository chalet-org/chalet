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

#include "Compile/CompileFactory.hpp"
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

		m_strategy = CompileFactory::makeStrategy(strategy, m_state);
		if (!m_strategy->initialize())
			return false;

		for (auto& project : m_state.projects)
		{
			if (project->cmake() || project->hasScripts())
				continue;

			if (!cacheRecipe(*project, inRoute))
				return false;
		}

		m_strategy->saveBuildFile();
	}

	bool multiTarget = m_state.projects.size() > 1;

	bool error = false;
	for (auto& project : m_state.projects)
	{
		if (runCommand && !project->runProject())
			continue;

		m_project = project.get();

		if (project->cmake())
		{
			if (!compileCMakeProject())
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

			const bool hasScripts = project->hasScripts();

			if (!runCommand || hasScripts)
			{
				if (!hasScripts)
					Output::msgTargetUpToDate(multiTarget, project->name());

				Output::print(Color::Reset, fmt::format("   Time: {}", buildTimer.asString()));
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
void BuildManager::printBuildInformation()
{
	m_state.compilerTools.fetchCompilerVersions();

	{
		const auto strategy = getBuildStrategyName();
		Diagnostic::info(fmt::format("Build Strategy: {}", strategy));
	}

	bool usingCpp = false;
	bool usingCc = false;
	for (auto& project : m_state.projects)
	{
		if (project->cmake() || project->hasScripts())
			continue;

		usingCpp |= project->language() == CodeLanguage::CPlusPlus;
		usingCc |= project->language() == CodeLanguage::C;
	}

	if (usingCpp && !m_state.compilerTools.compilerVersionStringCpp().empty())
	{
		Diagnostic::info(fmt::format("C++ Compiler: {}", m_state.compilerTools.compilerVersionStringCpp()));
	}
	if (usingCc && !m_state.compilerTools.compilerVersionStringC().empty())
	{
		Diagnostic::info(fmt::format("C Compiler: {}", m_state.compilerTools.compilerVersionStringC()));
	}
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
bool BuildManager::cacheRecipe(const ProjectConfiguration& inProject, const Route inRoute)
{
	auto& compilerConfig = m_state.compilerTools.getConfig(inProject.language());
	auto compilerType = compilerConfig.compilerType();

	auto buildToolchain = CompileFactory::makeToolchain(compilerType, m_state, inProject, compilerConfig);

	const bool objExtension = compilerType == CppCompilerType::VisualStudio;
	auto outputs = m_state.paths.getOutputs(inProject, compilerConfig.isMsvc(), objExtension);

	if (!Commands::makeDirectories(outputs.directories, m_cleanOutput))
	{
		Diagnostic::errorAbort(fmt::format("Error creating paths for project: {}", inProject.name()));
		return false;
	}

	if (!buildToolchain->preBuild())
	{
		Diagnostic::errorAbort(fmt::format("Error preparing the build for project: {}", inProject.name()));
		return false;
	}

	if (inRoute == Route::Rebuild)
	{
		doClean(inProject, outputs.target, outputs.objectList, outputs.dependencyList);
	}
	else if (inRoute == Route::BuildRun)
	{
		if (!copyRunDependencies(inProject))
		{
			Diagnostic::error(fmt::format("There was an error copying run dependencies for: {}", inProject.name()));
			return false;
		}
	}

	return m_strategy->addProject(inProject, std::move(outputs), buildToolchain);
}

/*****************************************************************************/
bool BuildManager::doScript()
{
	chalet_assert(m_project != nullptr, "");

	if (!m_project->hasScripts())
		return false;

	const auto& scripts = m_project->scripts();

	ScriptRunner scriptRunner(m_state.tools, m_inputs.buildFile(), m_cleanOutput);
	if (!scriptRunner.run(scripts))
	{
		Diagnostic::error(fmt::format("There was a problem running the script(s) for the build step: {}", m_project->name()));
		return false;
	}

	return true;
}

/*****************************************************************************/
bool BuildManager::copyRunDependencies(const ProjectConfiguration& inProject)
{
	bool result = true;

	if (inProject.name() == m_runProjectName)
	{
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
	}

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
bool BuildManager::doClean(const ProjectConfiguration& inProject, const std::string& inTarget, const StringList& inObjectList, const StringList& inDepList, const bool inFullClean)
{
	const auto& buildOutputDir = m_state.paths.buildOutputDir();

	if (m_cleanOutput && Commands::pathExists(buildOutputDir))
	{
		Output::msgCleaningRebuild();
		Output::lineBreak();
	}

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
bool BuildManager::cmdBuild()
{
	chalet_assert(m_project != nullptr, "");

	const auto& buildConfiguration = m_state.buildConfiguration();
	const auto& command = m_inputs.command();
	const auto& outputFile = m_project->outputFile();
	const bool hasScripts = m_project->hasScripts();

	if (hasScripts)
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
	if (hasScripts)
	{
		result = doScript();
	}
	else
	{
		result = m_strategy->buildProject(*m_project);
	}

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
bool BuildManager::cmdRebuild()
{
	chalet_assert(m_project != nullptr, "");

	const auto& buildConfiguration = m_state.buildConfiguration();
	const auto& outputFile = m_project->outputFile();

	Output::msgRebuild(buildConfiguration, outputFile);
	Output::lineBreak();

	if (!m_strategy->buildProject(*m_project))
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

	Timer buildTimer;

	m_state.tools.fetchCmakeVersion();

	CmakeBuilder cmake{ m_state, *m_project, m_cleanOutput };
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