/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Builder/BuildManager.hpp"

#include "Builder/BuildManager/CmakeBuilder.hpp"
#include "Router/Route.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Output.hpp"

#include "Compile/CompileFactory.hpp"
#include "Libraries/Format.hpp"
#include "Terminal/Environment.hpp"
#include "Terminal/Path.hpp"
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
		{ Route::Clean, &BuildManager::cmdClean },
		// { Route::kProfile, &BuildManager::cmdProfile },
		{ Route::Bundle, &BuildManager::cmdBuild },
	})
{
}

/*****************************************************************************/
bool BuildManager::run(const Route inRoute)
{
	if (m_buildRoutes.find(inRoute) == m_buildRoutes.end())
	{
		Diagnostic::error("Build command not recognized.");
		return false;
	}

	m_removeCache.clear();
	m_cleanOutput = m_state.environment.cleanOutput();

	m_runProjectName = getRunProject();

	bool runCommand = inRoute == Route::Run;
	bool cleanCommand = inRoute == Route::Clean;

	bool error = false;
	for (auto& project : m_state.projects)
	{
		int i = &project - &m_state.projects[0];
		if (cleanCommand && i > 0)
			break;

		if (!project->includeInBuild())
			continue;

		m_project = project.get();

		auto& compilerConfig = m_state.compilers.getConfig(m_project->language());
		m_state.environment.setPathVariable(compilerConfig);

		if (runCommand && !project->runProject())
			continue;

		if (project->cmake())
		{
			// TODO: refactor this and compileCMakeProject()

			Timer buildTimer;

			if (compileCMakeProject())
			{
				if (runCommand || cleanCommand)
					continue;

				auto result = buildTimer.stop();

				Output::print(Color::reset, fmt::format("   Build time: {}ms", result));
				Output::lineBreak();
			}
			else
				return false;
		}
		else
		{
			Timer buildTimer;

			// TODO: The original logic for this is weird because you could in theory
			//  build, then run, then build again. Separate out the "run" part
			if (!m_buildRoutes[inRoute](*this))
			{
				error = true;
				break;
			}

			if (!runCommand && !cleanCommand)
			{
				auto result = buildTimer.stop();

				Output::print(Color::reset, fmt::format("   Build time: {}ms", result));
				Output::lineBreak();
			}
		}

		m_project = nullptr;
	}

	if (error)
		return false;

	if (inRoute == Route::BuildRun || inRoute == Route::Build || inRoute == Route::Rebuild || cleanCommand)
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
	auto outputs = m_state.paths.getOutputs(*m_project);

	if (!Commands::makeDirectories(outputs.directories, m_cleanOutput))
	{
		Diagnostic::errorAbort(fmt::format("Error creating paths for project: {}\n  Aborting...", m_project->name()));
		return false;
	}

	if (inRoute == Route::Rebuild)
		doClean(outputs.objectList, outputs.dependencyList);

	const auto& preBuildScripts = m_project->preBuildScripts();
	const auto& postBuildScripts = m_project->postBuildScripts();

	// TODO: Check placement
	if (!runExternalScripts(preBuildScripts))
	{
		Diagnostic::errorAbort(fmt::format("There was a problem running the pre-build script for project: {}\n  Aborting...", m_project->name()));
		return false;
	}

	{
		auto& compilerConfig = m_state.compilers.getConfig(m_project->language());
		auto compilerType = compilerConfig.compilerType();
		auto buildToolchain = CompileFactory::makeToolchain(compilerType, m_state, *m_project, compilerConfig);

		auto strategyType = m_state.environment.strategy();
		auto buildStrategy = CompileFactory::makeStrategy(strategyType, m_state, *m_project, buildToolchain);

		if (!buildStrategy->createCache(outputs))
		{
			Diagnostic::errorAbort(fmt::format("Cache could not be created for project: {}\n  Aborting...", m_project->name()));
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
			Diagnostic::errorAbort(fmt::format("There was an error copying run dependencies for project: {}\n  Aborting...", m_project->name()));
			return false;
		}
	}

	// TODO: Always runs at the moment
	if (!runExternalScripts(postBuildScripts))
	{
		Diagnostic::errorAbort(fmt::format("There was a problem running the post-build script for project: {}\n  Aborting...", m_project->name()));
		return false;
	}

	//
	bool multiTarget = m_state.projects.size() > 1;
	Output::msgTargetUpToDate(multiTarget, m_project->name());

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
		const auto& buildDir = m_state.paths.buildDir();
		auto& compilerConfig = m_state.compilers.getConfig(m_project->language());
		auto runDependencies = getResolvedRunDependenciesList(compilerConfig);

		std::string outputFolder = fmt::format("{workingDirectory}/{buildDir}",
			FMT_ARG(workingDirectory),
			FMT_ARG(buildDir));

		int copied = 0;
		for (auto& dep : runDependencies)
		{
			std::string depFile = String::getPathFilename(dep);
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

		std::string resolved = fmt::format("{}/{}", compilerPathBin, dep);
		if (Commands::pathExists(resolved))
		{
			ret.push_back(std::move(resolved));
			continue;
		}

		for (auto& buildDep : m_state.environment.path())
		{
			resolved = fmt::format("{}/{}", buildDep, dep);
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
	std::string outputFile = getRunOutputFile();
	if (outputFile.empty())
		return false;

	const auto& workingDirectory = m_state.paths.workingDirectory();
	const auto& buildDir = m_state.paths.buildDir();
	const auto& runOptions = m_inputs.runOptions();

	const auto& runArguments = m_project->runArguments();

	// LOG(workingDirectory);

	std::string outputFolder = fmt::format("{workingDirectory}/{buildDir}",
		FMT_ARG(workingDirectory),
		FMT_ARG(buildDir));

	Output::msgLaunch(buildDir, outputFile);
	Output::lineBreak();

	// LOG(runOptions);
	// LOG(runArguments);

	std::string file = fmt::format("{outputFolder}/{outputFile}",
		FMT_ARG(outputFolder),
		FMT_ARG(outputFile));

	// LOG(file);

	if (!Commands::pathExists(file))
		return false;

	std::string args = !runOptions.empty() ? runOptions : runArguments;
	std::string cmd = fmt::format("{} {}", file, args);

	// LOG(cmd);

	return Commands::shell(cmd);
}

/*****************************************************************************/
bool BuildManager::doLazyClean()
{
	chalet_assert(m_project != nullptr, "");

	const auto& buildDir = m_state.paths.buildDir();
	const auto& binDir = m_state.paths.binDir();

	if (!Commands::pathExists(buildDir))
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

	const auto& build = m_state.buildConfiguration();

	if (build.empty())
		Commands::removeRecursively(binDir, m_cleanOutput);
	else
		Commands::removeRecursively(buildDir, m_cleanOutput);

	// TODO: Clean CMake projects

	if (!m_cleanOutput)
		Output::lineBreak();

	return true;
}

/*****************************************************************************/
bool BuildManager::doClean(const StringList& inObjectList, const StringList& inDepList, const bool inFullClean)
{
	chalet_assert(m_project != nullptr, "");

	const auto& buildDir = m_state.paths.buildDir();

	if (m_cleanOutput && Commands::pathExists(buildDir))
	{
		Output::msgCleaningRebuild();
		Output::lineBreak();
	}

	std::string pch = m_state.paths.getPrecompiledHeader(*m_project);

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

	if (m_project->name() == m_runProjectName && command == Route::BuildRun)
		Output::msgBuildAndRun(buildConfiguration, outputFile);
	else
		Output::msgBuild(buildConfiguration, outputFile);

	Output::lineBreak();

	if (!doBuild(command))
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
	const auto& outputFile = m_project->outputFile();

	Output::msgRun(buildConfiguration, outputFile);
	Output::lineBreak();

	return true;
}

/*****************************************************************************/
bool BuildManager::cmdClean()
{
	chalet_assert(m_project != nullptr, "");

	const auto& buildConfiguration = m_state.buildConfiguration();

	Output::msgClean(buildConfiguration);
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
		if (!Commands::pathExists(scriptPath))
		{
			Diagnostic::warn(fmt::format("{}: The script '{}' was not found. Skipping.", CommandLineInputs::file(), scriptPath));
			continue;
		}

		result &= Commands::setExecutableFlag(scriptPath, m_cleanOutput);
		result &= Commands::shell(fmt::format("'{}'", scriptPath), m_cleanOutput);
		Output::lineBreak();
	}

	return result;
}

/*****************************************************************************/
void BuildManager::testTerminalMessages()
{
	const auto& buildConfiguration = m_state.buildConfiguration();
	const auto& distConfig = m_state.bundle.configuration();
	const auto& buildDir = m_state.paths.buildDir();

	const std::string name{ "cool-program.exe" };
	const std::string profAnalysis{ "profiler_analysis.stats" };

	Output::msgBuildSuccess();
	Output::msgLaunch(buildDir, name);
	Output::msgBuildFail();
	Output::msgBuildProdError(distConfig);
	Output::msgProfilerDone(profAnalysis);
	Output::msgProfilerError();
	Output::msgProfilerErrorMacOS();
	Output::msgBuildAndRun(buildConfiguration, name);
	Output::msgBuild(buildConfiguration, name);
	Output::msgRebuild(buildConfiguration, name);
	Output::msgRun(buildConfiguration, name);
	Output::msgBuildProd(buildConfiguration, name);
	Output::msgProfile(buildConfiguration, name);
}
}