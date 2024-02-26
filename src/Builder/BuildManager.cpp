/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Builder/BuildManager.hpp"

#include "BuildEnvironment/IBuildEnvironment.hpp"
#include "Builder/BatchValidator.hpp"
#include "Builder/CmakeBuilder.hpp"
#include "Builder/ConfigureFileParser.hpp"
#include "Builder/ProfilerRunner.hpp"
#include "Builder/ScriptRunner.hpp"
#include "Builder/SubChaletBuilder.hpp"
#include "Bundler/BinaryDependency/BinaryDependencyMap.hpp"
#include "Cache/WorkspaceCache.hpp"
#include "Compile/AssemblyDumper.hpp"
#include "Compile/CompileToolchainController.hpp"
#include "Core/CommandLineInputs.hpp"
#include "Process/Environment.hpp"
#include "Process/Process.hpp"
#include "Process/SubProcessController.hpp"
#include "State/AncillaryTools.hpp"
#include "State/BuildConfiguration.hpp"
#include "State/BuildInfo.hpp"
#include "State/BuildPaths.hpp"
#include "State/BuildState.hpp"
#include "State/CompilerTools.hpp"
#include "State/Distribution/IDistTarget.hpp"
#include "State/Target/CMakeTarget.hpp"
#include "State/Target/ProcessBuildTarget.hpp"
#include "State/Target/ScriptBuildTarget.hpp"
#include "State/Target/SourceTarget.hpp"
#include "State/Target/SubChaletTarget.hpp"
#include "State/Target/ValidationBuildTarget.hpp"
#include "State/TargetMetadata.hpp"
#include "State/WorkspaceEnvironment.hpp"
#include "System/Files.hpp"
#include "Terminal/Output.hpp"
#include "Terminal/Unicode.hpp"
#include "Terminal/WindowsTerminal.hpp"
#include "Utility/List.hpp"
#include "Utility/Path.hpp"
#include "Utility/String.hpp"
#include "Utility/Timer.hpp"
#include "Json/JsonKeys.hpp"
#include "Json/JsonValues.hpp"

#include "Json/JsonComments.hpp"

namespace chalet
{
/*****************************************************************************/
BuildManager::BuildManager(BuildState& inState) :
	m_state(inState),
	m_buildRoutes({
		{ RouteType::BuildRun, &BuildManager::cmdBuild },
		{ RouteType::Build, &BuildManager::cmdBuild },
		{ RouteType::Rebuild, &BuildManager::cmdRebuild },
		{ RouteType::Run, &BuildManager::cmdRun },
		{ RouteType::Bundle, &BuildManager::cmdBuild },
	})
{
}

/*****************************************************************************/
BuildManager::~BuildManager() = default;

/*****************************************************************************/
void BuildManager::populateBuildTargets(const CommandRoute& inRoute)
{
	UNUSED(inRoute);
	m_buildTargets.clear();

	auto buildTargets = m_state.inputs.getBuildTargets();
	const bool addAllTargets = List::contains(buildTargets, std::string(Values::All)) || !m_state.info.onlyRequired();

	StringList requiredTargets;
	if (!addAllTargets)
	{
		for (auto& target : buildTargets)
		{
			m_state.getTargetDependencies(requiredTargets, target, true);
		}
	}

	for (auto& target : m_state.targets)
	{
		auto& targetName = target->name();
		if (!addAllTargets && target->isSources() && !List::contains(requiredTargets, targetName))
			continue;

		m_buildTargets.emplace_back(target.get());
	}
}

/*****************************************************************************/
const IBuildTarget* BuildManager::getRunTarget(const CommandRoute& inRoute)
{
	if (inRoute.willRun())
		return m_state.getFirstValidRunTarget();

	return nullptr;
}

/*****************************************************************************/
bool BuildManager::run(const CommandRoute& inRoute, const bool inShowSuccess)
{
	m_timer.restart();

	m_strategy = ICompileStrategy::make(m_state.toolchain.strategy(), m_state);

	if (m_state.cache.file().canWipeBuildFolder())
	{
		Files::removeRecursively(m_state.paths.buildOutputDir());
	}

	populateBuildTargets(inRoute);
	auto runTarget = getRunTarget(inRoute);

	if (inRoute.isClean())
	{
		Output::lineBreak();

		if (!cmdClean())
			return false;

		Output::msgBuildSuccess();
		Output::lineBreak();
		return true;
	}
	else if (inRoute.isRebuild())
	{
		// Don't produce any output from this
		doFullBuildFolderClean(false, false);
	}

	if (!checkIntermediateFiles())
	{
		Diagnostic::error("Failed to generate needed intermediate files");
		return false;
	}

	if (m_buildRoutes.find(inRoute.type()) == m_buildRoutes.end())
	{
		Output::lineBreak();
		Diagnostic::error("Build command not recognized.");
		return false;
	}

	const bool runRoute = inRoute.isRun();
	const bool routeWillRun = inRoute.willRun();

	std::string runTargetName;
	if (runTarget != nullptr)
		runTargetName = runTarget->name();

	if (!runRoute && m_state.toolchain.strategy() != StrategyType::Native)
	{
		printBuildInformation();
		Output::lineBreak();
	}

	// Note: We still have to initialize the build when the command is "run"
	if (!m_strategy->initialize())
		return false;

	if (!runRoute)
	{
		Timer prepTimer;
		if (m_state.toolchain.strategy() == StrategyType::Native)
		{
			Diagnostic::infoEllipsis("Resolving source file dependencies");
		}

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
			else if (inRoute.isRebuild())
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

		if (m_state.toolchain.strategy() == StrategyType::Native)
		{
			Diagnostic::printDone(prepTimer.asString());
			printBuildInformation();
			Output::lineBreak();
			m_timer.restart();
		}
	}

	m_strategy->doPreBuild();
	m_fileCache.clear();

	if (inRoute.isRebuild())
	{
		if (Output::showCommands())
			Output::lineBreak();
	}

	bool error = false;

	bool buildAll = m_strategy->isMSBuild() || m_strategy->isXcodeBuild();
	bool multiTarget = m_buildTargets.size() > 1;
	for (auto& target : m_buildTargets)
	{
		if (routeWillRun)
		{
			bool isRunTarget = !runTargetName.empty() && String::equals(runTargetName, target->name());
			bool noExplicitRunTarget = runTargetName.empty() && runTarget == nullptr;

			if (isRunTarget || noExplicitRunTarget)
			{
				if (target->isScript() || target->isProcess())
					break;
			}

			if (runRoute || buildAll)
				continue;
		}
		else if (buildAll)
			break;

		// At this point, we build
		bool result = false;
		if (target->isSubChalet())
		{
			result = runSubChaletTarget(static_cast<const SubChaletTarget&>(*target));
		}
		else if (target->isCMake())
		{
			result = runCMakeTarget(static_cast<const CMakeTarget&>(*target));
		}
		else if (target->isScript())
		{
			result = runScriptTarget(static_cast<const ScriptBuildTarget&>(*target), false);
		}
		else if (target->isProcess())
		{
			result = runProcessTarget(static_cast<const ProcessBuildTarget&>(*target));
		}
		else if (target->isValidation())
		{
			result = runValidationTarget(static_cast<const ValidationBuildTarget&>(*target));
		}
		else
		{
			Timer buildTimer;

			result = m_buildRoutes[inRoute.type()](*this, static_cast<const SourceTarget&>(*target));

			if (result)
			{
				Output::msgTargetUpToDate(multiTarget, target->name());
				stopTimerAndShowBenchmark(buildTimer);
			}
		}

		if (!result)
		{
			error = true;
			break;
		}

		Output::lineBreak();

		// if (breakAfterBuild)
		// 	break;
	}

	if (buildAll && !runRoute)
	{
		error = !runFullBuild();
		Output::lineBreak();
	}

	for (auto& target : m_state.targets)
	{
		if (target->isSources())
		{
			if (!onFinishBuild(static_cast<const SourceTarget&>(*target)))
				return false;
		}
	}

	if (!runRoute)
	{
		if (!m_strategy->doPostBuild())
		{
			Diagnostic::error("The post-build step encountered a problem.");
			return false;
		}
	}

	if (error)
	{
		if (!runRoute && !m_state.isSubChaletTarget())
		{
			Output::msgBuildFail();
			Output::lineBreak();
		}
		return false;
	}
	else if (!runRoute && inShowSuccess)
	{
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

		if (!inRoute.isBuildRun())
			Output::lineBreak();
	}

	m_state.makeLibraryPathVariables();

	if (routeWillRun)
	{
		if (runTarget == nullptr)
		{
			Diagnostic::error("No executable project was found to run.");
			return false;
		}
		else if (runTarget->isSources() || runTarget->isCMake())
		{
			Output::lineBreak();
			return cmdRun(*runTarget);
		}
		else if (runTarget->isScript())
		{
			Output::lineBreak();
			return runScriptTarget(static_cast<const ScriptBuildTarget&>(*runTarget), true);
		}
		else if (runTarget->isProcess())
		{
			Output::lineBreak();
			return runProcessTarget(static_cast<const ProcessBuildTarget&>(*runTarget));
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
	bool usingObjectiveCpp = false;
	bool usingObjectiveC = false;
	bool usingCpp = false;
	bool usingC = false;
	for (auto& target : m_buildTargets)
	{
		if (target->isSources())
		{
			auto& project = static_cast<const SourceTarget&>(*target);
			auto language = project.language();

			usingObjectiveCpp |= language == CodeLanguage::ObjectiveCPlusPlus;
			usingObjectiveC |= language == CodeLanguage::ObjectiveC;
			usingCpp |= project.language() == CodeLanguage::CPlusPlus;
			usingC |= project.language() == CodeLanguage::C;
		}
	}

	auto printDetailsImpl = [](const CompilerInfo& inInfo, const std::string& inLang) -> void {
		if (inInfo.description.empty())
			return;

		Diagnostic::info("{} Compiler: {}", inLang, inInfo.description);
	};

	if (usingObjectiveCpp)
		printDetailsImpl(m_state.toolchain.compilerCpp(), "Objective-C++");

	if (usingObjectiveC)
		printDetailsImpl(m_state.toolchain.compilerCpp(), "Objective-C");

	if (usingCpp)
		printDetailsImpl(m_state.toolchain.compilerCpp(), "C++");

	if (usingC)
		printDetailsImpl(m_state.toolchain.compilerC(), "C");

	auto& machineArch = m_state.inputs.hostArchitecture();
	const auto& hostArch = m_state.info.hostArchitectureString();
	if (hostArch != machineArch)
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

#if defined(CHALET_WIN32)
		case StrategyType::MSBuild:
			ret = "MSBuild";
			break;
#elif defined(CHALET_MACOS)
		case StrategyType::XcodeBuild:
			ret = "XcodeBuild";
			break;
#endif

		case StrategyType::Makefile: {
#if defined(CHALET_WIN32)
			if (m_state.toolchain.makeIsNMake())
			{
				if (m_state.toolchain.makeIsJom())
					ret = "NMAKE (Qt Jom)";
				else
					ret = "NMAKE";
			}
			else
#endif
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
	auto& fileCache = m_fileCache[inProject.buildSuffix()];
	auto outputs = m_state.paths.getOutputs(inProject, fileCache);

	// Note: no output from this for now
	if (!Files::makeDirectories(outputs->directories))
	{
		Diagnostic::error("Error creating paths for project: {}", inProject.name());
		return false;
	}

	if (!inProject.configureFiles().empty())
	{
		if (!runConfigureFileParser(inProject, m_state.paths.intermediateDir()))
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
bool BuildManager::copyRunDependencies(const IBuildTarget& inTarget, u32& outCopied)
{
	bool result = true;

	if (inTarget.isSources())
	{
		const auto& sourceTarget = static_cast<const SourceTarget&>(inTarget);

		std::string cwd = m_state.inputs.workingDirectory() + '/';

		const auto& buildOutputDir = m_state.paths.buildOutputDir();
		auto copyFilesOnRun = sourceTarget.getResolvedRunDependenciesList();
		for (auto& dep : copyFilesOnRun)
		{
			auto depFile = String::getPathFilename(dep);
			if (!Files::pathExists(fmt::format("{}/{}", buildOutputDir, depFile)))
			{
				result &= Files::copyIfDoesNotExistWithoutPrintingWorkingDirectory(dep, buildOutputDir, cwd);
				++outCopied;
			}
		}
	}

	return result;
}

/*****************************************************************************/
bool BuildManager::runProfiler(const SourceTarget& inProject, const StringList& inCommand, const std::string& inExecutable)
{
	ProfilerRunner profiler(m_state, inProject);
	return profiler.run(inCommand, inExecutable);
}

/*****************************************************************************/
bool BuildManager::runConfigureFileParser(const SourceTarget& inProject, const std::string& outFolder) const
{
	ConfigureFileParser confFileParser(m_state, inProject);
	return confFileParser.run(outFolder);
}

/*****************************************************************************/
bool BuildManager::doFullBuildFolderClean(const bool inShowMessage, const bool inCleanExternals)
{
	auto buildOutputDir = m_state.paths.buildOutputDir();

	auto dirToClean = buildOutputDir;

	Timer timer;

	if (inShowMessage && Output::cleanOutput())
	{
		Diagnostic::stepInfoEllipsis("Removing build files & folders");
	}

	bool didClean = false;

	StringList buildDirs;
	StringList externalLocations;
	for (const auto& target : m_buildTargets)
	{
		if (target->isSources())
		{
			auto dirs = m_state.paths.getBuildDirectories(static_cast<const SourceTarget&>(*target));
			for (auto&& dir : dirs)
			{
				List::addIfDoesNotExist(buildDirs, std::move(dir));
			}
		}
		else if (target->isSubChalet())
		{
			auto& subChaletTarget = static_cast<const SubChaletTarget&>(*target);
			if (inCleanExternals)
				didClean |= doSubChaletClean(subChaletTarget);

			List::addIfDoesNotExist(externalLocations, subChaletTarget.targetFolder());
		}
		else if (target->isCMake())
		{
			auto& cmakeTarget = static_cast<const CMakeTarget&>(*target);
			if (inCleanExternals)
				didClean |= doCMakeClean(cmakeTarget);

			List::addIfDoesNotExist(externalLocations, cmakeTarget.targetFolder());
		}
	}

	for (const auto& target : m_state.distribution)
	{
		if (target->isDistributionBundle())
		{
			List::addIfDoesNotExist(buildDirs, m_state.paths.bundleObjDir(target->name()));

#if defined(CHALET_MACOS)
			if (m_state.toolchain.strategy() == StrategyType::XcodeBuild)
			{
				List::addIfDoesNotExist(buildDirs, fmt::format("{}/{}.app", buildOutputDir, target->name()));
			}
#endif
		}
	}

	buildOutputDir += '/';

	for (auto& location : externalLocations)
	{
		String::replaceAll(location, buildOutputDir, "");
	}

	bool dirExists = Files::pathExists(dirToClean);
	bool nothingToClean = !dirExists && !didClean;
	if (dirExists)
	{
		fs::recursive_directory_iterator it(dirToClean);
		fs::recursive_directory_iterator itEnd;

		while (it != itEnd)
		{
			auto shortPath = it->path().string();
			Path::toUnix(shortPath);

			String::replaceAll(shortPath, buildOutputDir, "");

			if (!String::startsWith(externalLocations, shortPath))
			{
				auto pth = it->path();
				++it;

				std::error_code ec;
				fs::remove(pth, ec);
			}
			else
			{
				++it;
			}
		}
	}

	for (auto& dir : buildDirs)
	{
		if (Files::pathExists(dir))
			Files::removeRecursively(dir);
	}

	auto ccmdsJson = m_state.paths.currentCompileCommands();
	Files::removeIfExists(ccmdsJson);

	if (Files::pathIsEmpty(buildOutputDir))
		Files::removeIfExists(buildOutputDir);

	if (inShowMessage && Output::cleanOutput())
	{
		Diagnostic::printDone(timer.asString());
		UNUSED(nothingToClean);
		Output::lineBreak();
	}

	return true;
}

/*****************************************************************************/
bool BuildManager::checkIntermediateFiles() const
{
	bool isPlatformProjectBuild = m_strategy->isXcodeBuild() || m_strategy->isMSBuild();
	for (auto& target : m_state.targets)
	{
		if (!target->isSources())
			continue;

		const auto& project = static_cast<const SourceTarget&>(*target);
		if (isPlatformProjectBuild && !project.configureFiles().empty())
		{
			auto outFolder = m_state.paths.intermediateDir();
			// #if defined(CHALET_MACOS)
			// 			if (m_strategy->isXcodeBuild())
			// 			{
			// 				outFolder = fmt::format("{}/obj.{}", m_state.paths.buildOutputDir(), project.name());
			// 			}
			// #endif
			runConfigureFileParser(project, outFolder); // ignore the result
		}

		if (!isPlatformProjectBuild && project.unityBuild())
		{
			std::string unityBuildFilename;
			if (!project.generateUnityBuildFile(unityBuildFilename))
				return false;
		}
	}

	return true;
}

/*****************************************************************************/
bool BuildManager::doSubChaletClean(const SubChaletTarget& inTarget)
{
	auto targetFolder = inTarget.targetFolder();
	Path::toUnix(targetFolder);

	bool clean = m_state.inputs.route().isClean() && inTarget.clean();
	bool rebuild = m_state.inputs.route().isRebuild() && inTarget.rebuild();

	if (clean)
	{
		SubChaletBuilder subChalet(m_state, inTarget);
		subChalet.removeSettingsFile();
	}

	if ((clean || rebuild) && Files::pathExists(targetFolder))
	{
		if (!Files::removeRecursively(targetFolder))
		{
			Diagnostic::error("There was an error rebuilding the '{}' Chalet project.", inTarget.name());
			return false;
		}
	}

	return true;
}

/*****************************************************************************/
bool BuildManager::doCMakeClean(const CMakeTarget& inTarget)
{
	auto targetFolder = inTarget.targetFolder();
	Path::toUnix(targetFolder);

	bool clean = m_state.inputs.route().isClean() && inTarget.clean();
	bool rebuild = m_state.inputs.route().isRebuild() && inTarget.rebuild();

	if ((clean || rebuild) && Files::pathExists(targetFolder))
	{
		if (!Files::removeRecursively(targetFolder))
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

	Timer buildTimer;
	bool result = true;

	const Color color = inRunCommand ? Output::theme().success : Output::theme().header;

	if (!inTarget.outputDescription().empty())
		Output::msgTargetDescription(inTarget.outputDescription(), color);
	else
		Output::msgTargetOfType("Script", inTarget.name(), color);

	if (inRunCommand)
		Output::printSeparator();
	else
		Output::lineBreak();

	const auto& arguments = inTarget.arguments();
	ScriptRunner scriptRunner(m_state.inputs, m_state.tools);
	if (!scriptRunner.run(inTarget.scriptType(), file, arguments, inRunCommand))
	{
		if (!inRunCommand)
			Output::previousLine();

		Diagnostic::printErrors(true);
		result = false;
	}

	if (!inRunCommand && result)
		stopTimerAndShowBenchmark(buildTimer);

	return result;
}

/*****************************************************************************/
bool BuildManager::runProcessTarget(const ProcessBuildTarget& inTarget)
{
	const auto& path = inTarget.path();
	if (path.empty())
		return false;

	Timer buildTimer;

	displayHeader("Process", inTarget);

	StringList cmd;
	cmd.push_back(inTarget.path());
	for (auto& arg : inTarget.arguments())
	{
		cmd.push_back(arg);
	}

	bool result = runProcess(cmd, inTarget.path(), false);

	if (!result)
		Output::lineBreak();

	stopTimerAndShowBenchmark(buildTimer);

	return result;
}

/*****************************************************************************/
bool BuildManager::runValidationTarget(const ValidationBuildTarget& inTarget)
{
	const auto& schema = inTarget.schema();
	if (schema.empty())
		return false;

	Timer buildTimer;

	displayHeader("Validation", inTarget);

	BatchValidator validator(&m_state, inTarget.schema());
	bool result = validator.validate(inTarget.files());

	stopTimerAndShowBenchmark(buildTimer);

	if (!result)
		Output::lineBreak();

	return result;
}

/*****************************************************************************/
void BuildManager::displayHeader(const std::string& inLabel, const IBuildTarget& inTarget, const std::string& inName) const
{
	if (!inTarget.outputDescription().empty())
		Output::msgTargetDescription(inTarget.outputDescription(), Output::theme().header);
	else
		Output::msgTargetOfType(inLabel, !inName.empty() ? inName : inTarget.name(), Output::theme().header);

	Output::lineBreak();
}

/*****************************************************************************/
bool BuildManager::cmdBuild(const SourceTarget& inProject)
{
	const auto& outputFile = inProject.outputFile();

	displayHeader("Build", inProject, outputFile);

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
		if (!m_asmDumper->dumpProject(inProject, fileCache))
			return false;
	}

	return true;
}

/*****************************************************************************/
bool BuildManager::onFinishBuild(const SourceTarget& inProject) const
{
	UNUSED(inProject);
	const auto& buildOutputDir = m_state.paths.buildOutputDir();

	if (Files::pathExists(buildOutputDir))
	{
		std::error_code ec;
		fs::recursive_directory_iterator it(buildOutputDir);
		fs::recursive_directory_iterator itEnd;

		while (it != itEnd)
		{
			const auto& path = it->path();

			if (it->is_directory() && fs::is_empty(path, ec))
			{
				auto pth = path;
				++it;

				ec.clear();
				fs::remove(pth, ec);
				ec.clear();
			}
			else
			{
				++it;
			}
		}
	}

	return true;
}

/*****************************************************************************/
bool BuildManager::cmdRebuild(const SourceTarget& inProject)
{
	const auto& outputFile = inProject.outputFile();

	displayHeader("Rebuild", inProject, outputFile);

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
		bool forced = true;
		if (!m_asmDumper->dumpProject(inProject, fileCache, forced))
			return false;
	}

	return true;
}

/*****************************************************************************/
bool BuildManager::cmdRun(const IBuildTarget& inTarget)
{
	std::string outputFile;
	if (inTarget.isSources())
	{
		auto& project = static_cast<const SourceTarget&>(inTarget);
		outputFile = m_state.paths.getTargetFilename(project);
	}
	else if (inTarget.isCMake())
	{
		auto& project = static_cast<const CMakeTarget&>(inTarget);
		outputFile = m_state.paths.getTargetFilename(project);
	}

	if (Files::pathIsDirectory(outputFile))
	{
		Diagnostic::error("Requested run target '{}' resolves to a directory: {}", inTarget.name(), outputFile);
		return false;
	}

	if (outputFile.empty() || !Files::pathExists(outputFile))
	{
		Diagnostic::error("Requested configuration '{}' must be built for run target: '{}'", m_state.info.buildConfiguration(), inTarget.name());
		return false;
	}

	const auto& runArguments = m_state.inputs.runArguments();

	auto file = Files::getAbsolutePath(outputFile);
	if (!Files::pathExists(file))
	{
		Diagnostic::error("Couldn't find file: {}", file);
		return false;
	}

	u32 copied = 0;
	for (const auto& target : m_buildTargets)
	{
		if (target->isSources())
		{
			auto& project = static_cast<const SourceTarget&>(*target);
			if (!project.copyFilesOnRun().empty())
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
	}

	if (copied > 0)
		Output::lineBreak();

	//

	if (!inTarget.outputDescription().empty())
		Output::msgTargetDescription(inTarget.outputDescription(), Output::theme().success);
	else
		Output::msgRun(outputFile);

	StringList cmd;
	if (m_state.environment->isEmscripten())
	{
		auto outputHtml = outputFile;
		outputFile = fmt::format("{}/index.html", String::getPathFolder(outputFile));
		Files::copyRename(outputHtml, outputFile, true);

		auto pythonPath = Environment::getString("EMSDK_PYTHON");
		auto upstream = Environment::getString("EMSDK_UPSTREAM_EMSCRIPTEN");
		auto port = Environment::getString("EMRUN_PORT");
		auto emrun = fmt::format("{}/emrun.py", upstream);

		cmd.emplace_back(std::move(pythonPath));
		cmd.emplace_back(std::move(emrun));

		cmd.emplace_back("--no_browser");
		cmd.emplace_back("--serve_after_close");
		cmd.emplace_back("--serve_after_exit");
		cmd.emplace_back("--no_emrun_detect");

		if (Output::showCommands())
			cmd.emplace_back("--verbose");

		cmd.emplace_back("--hostname");
		cmd.emplace_back("localhost");
		cmd.emplace_back("--port");
		cmd.emplace_back(port);

		if (m_state.configuration.debugSymbols())
			cmd.emplace_back(m_state.inputs.workingDirectory());
		else
			cmd.emplace_back(file);

		if (runArguments.has_value() && !runArguments->empty())
			cmd.emplace_back("--");
	}
	else
	{
		cmd.emplace_back(file);
	}

	if (runArguments.has_value())
	{
		for (auto& arg : *runArguments)
			cmd.push_back(arg);
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
	{
		Output::printSeparator();

		if (m_state.environment->isEmscripten())
		{
			auto cancelCmd = "CTRL+C";
			Output::print(Output::theme().flair, fmt::format("(Press {} to exit the server)", cancelCmd));

			auto port = Environment::getString("EMRUN_PORT");
			if (m_state.configuration.debugSymbols())
				Output::print(Output::theme().info, fmt::format("Navigate to: http://localhost:{}/{}\n", port, m_state.paths.buildOutputDir()));
			else
				Output::print(Output::theme().info, fmt::format("Navigate to: http://localhost:{}\n", port));
		}
	}

#if defined(CHALET_WIN32)
	WindowsTerminal::cleanup();
#endif

	bool result = Process::runWithInput(inCmd);

#if defined(CHALET_WIN32)
	WindowsTerminal::initialize();
#endif

	m_state.inputs.clearWorkingDirectory(outputFile);

	i32 lastExitCode = SubProcessController::getLastExitCode();
	i32 signalRaised = 0;

	std::string signalRaisedMessage;
	if (lastExitCode < 0)
	{
		signalRaisedMessage = SubProcessController::getSignalRaisedMessage(lastExitCode);
		if (!signalRaisedMessage.empty())
		{
			signalRaised = lastExitCode * -1;
			lastExitCode = 1;
		}
	}

	if (lastExitCode != 0 || inFromDist)
	{
		auto message = fmt::format("{} exited with code: {}", outputFile, lastExitCode);

		if (inFromDist)
			Output::printSeparator();

		Output::print(result ? Output::theme().info : Output::theme().error, message);
	}

	std::string lastSystemMessage;
	if (signalRaised == 0)
	{
		lastSystemMessage = SubProcessController::getSystemMessage(lastExitCode);
	}
	if (!lastSystemMessage.empty())
	{
#if defined(CHALET_WIN32)
		String::replaceAll(lastSystemMessage, "%1", outputFile);
#endif
		Output::print(Output::theme().info, fmt::format("Error: {}", lastSystemMessage));
	}
	else if (!signalRaisedMessage.empty())
	{
		auto signalName = SubProcessController::getSignalNameFromCode(signalRaised);
		Output::print(Output::theme().info, fmt::format("Error: {} [{}] - {}", signalName, signalRaised, signalRaisedMessage));
	}
	else if (lastExitCode != 0)
	{
		BinaryDependencyMap tmpMap(m_state);
		StringList dependencies;
		StringList dependenciesNotFound;

		tmpMap.setIncludeWinUCRT(true);
		if (tmpMap.getExecutableDependencies(outputFile, dependencies, &dependenciesNotFound) && !dependenciesNotFound.empty())
		{
			for (auto& dep : dependenciesNotFound)
			{
				Output::print(Output::theme().info, fmt::format("Error: Cannot open shared object file: {}: No such file or directory.", dep));
			}
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

	if (!doFullBuildFolderClean(true, true))
		return true;

	if (Output::showCommands())
		Output::lineBreak();

	return true;
}

/*****************************************************************************/
bool BuildManager::runSubChaletTarget(const SubChaletTarget& inTarget)
{
	Timer buildTimer;

	displayHeader("Build", inTarget);

	SubChaletBuilder subChalet(m_state, inTarget);
	if (!subChalet.run())
		return false;

	auto result = buildTimer.stop();
	if (result > 0 && Output::showBenchmarks())
	{
		Output::printInfo(fmt::format("   Time: {}", buildTimer.asString()));
	}

	return true;
}

/*****************************************************************************/
bool BuildManager::runCMakeTarget(const CMakeTarget& inTarget)
{
	Timer buildTimer;

	displayHeader("Build", inTarget);

	CmakeBuilder cmake(m_state, inTarget);
	if (!cmake.run())
		return false;

	stopTimerAndShowBenchmark(buildTimer);

	return true;
}

/*****************************************************************************/
bool BuildManager::runFullBuild()
{
	// Timer buildTimer;

	const auto& workspace = m_state.workspace.metadata().name();

	if (m_state.inputs.route().isRebuild())
		Output::msgTargetOfType("Rebuild", workspace, Output::theme().header);
	else
		Output::msgTargetOfType("Build", workspace, Output::theme().header);

	Output::lineBreak();

	if (!m_strategy->doFullBuild())
		return false;

	// stopTimerAndShowBenchmark(buildTimer);

	return true;
}

/*****************************************************************************/
void BuildManager::stopTimerAndShowBenchmark(Timer& outTimer)
{
	int64_t result = outTimer.stop();
	if (result > 0 && Output::showBenchmarks())
	{
		Output::printInfo(fmt::format("   Time: {}", outTimer.asString()));
	}
}
}