/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Builder/ProfilerRunner.hpp"

#include "BuildEnvironment/IBuildEnvironment.hpp"
#include "Core/CommandLineInputs.hpp"
#include "Process/Environment.hpp"
#include "Process/Process.hpp"
#include "Process/SubProcessController.hpp"
#include "State/AncillaryTools.hpp"
#include "State/BuildInfo.hpp"
#include "State/BuildPaths.hpp"
#include "State/BuildState.hpp"
#include "State/CompilerTools.hpp"
#include "State/Target/SourceTarget.hpp"
#include "System/Files.hpp"
#include "Terminal/Output.hpp"
#include "Terminal/WindowsTerminal.hpp"
#include "Utility/Path.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
// TODO: This whole thing needs a proper refactor
//
ProfilerRunner::ProfilerRunner(BuildState& inState, const SourceTarget& inProject) :
	m_state(inState),
	m_project(inProject)
{
}

/*****************************************************************************/
bool ProfilerRunner::run(const StringList& inCommand, const std::string& inExecutable)
{
	const auto& profiler = m_state.toolchain.profiler();
	if (!profiler.empty() && m_state.toolchain.isProfilerGprof())
	{
		return runWithGprof(inCommand, inExecutable);
	}

#if defined(CHALET_MACOS)
	if (m_state.environment->isAppleClang())
	{
		/*
			Notes:
				Nice resource on the topic of profiling in mac:
				https://gist.github.com/loderunner/36724cc9ee8db66db305

			sudo xcode-select -s /Library/Developer/CommandLineTools
			sudo xcode-select -s /Applications/Xcode.app/Contents/Developer

			'xcrun xctrace' should be the standard from Xcode 12 onward (until it changes again),
			and it superscedes the 'instruments' command-line util

			CommandLineTools might not have access to instruments (at least in 12), or xcrun xctrace
			'sample' will need to be used instead if only CommandLineTools is selected.
			sample requires the PID (get from subprocess somehow), while both flavors of making an
			Instruments trace can be passed the commands directly
		*/

		bool xctraceAvailable = false;
		if (!m_state.tools.xcrun().empty())
		{
			const auto xctraceOutput = Process::runOutput({ m_state.tools.xcrun(), "xctrace" });
			xctraceAvailable = !String::contains("unable to find utility", xctraceOutput);
		}
		const bool useXcTrace = m_state.tools.xcodeVersionMajor() >= 12 || xctraceAvailable;

		bool instrumentsAvailable = m_state.tools.xcodeVersionMajor() <= 12;
		if (instrumentsAvailable && !m_state.tools.instruments().empty())
		{
			const auto instrumentsOutput = Process::runOutput({ m_state.tools.instruments() });
			instrumentsAvailable = !String::contains("requires Xcode", instrumentsOutput);
		}

		if (xctraceAvailable || instrumentsAvailable)
		{
			return runWithInstruments(inCommand, inExecutable, useXcTrace);
		}
		else
		{
			return runWithSample(inCommand, inExecutable);
		}
	}
#endif

#if defined(CHALET_WIN32)
	if (!profiler.empty() && m_state.environment->isMsvc())
	{
		if (m_state.toolchain.isProfilerVSDiagnostics())
			return runWithVisualStudioDiagnostics(inCommand, inExecutable);

		if (m_state.toolchain.isProfilerVSInstruments())
			return runWithVisualStudioInstruments(inCommand, inExecutable);
	}
#endif

	// Not supported - This should be validated in BuildState::validate()
	return false;
}

/*****************************************************************************/
void ProfilerRunner::printExitedWithCode(const bool inResult) const
{
	auto outFile = m_state.paths.getTargetFilename(m_project);
	m_state.inputs.clearWorkingDirectory(outFile);

	auto message = fmt::format("{} exited with code: {}", outFile, SubProcessController::getLastExitCode());

	// Output::lineBreak();
	Output::printSeparator();
	Output::print(inResult ? Output::theme().info : Output::theme().error, message);
	Output::lineBreak();
}

/*****************************************************************************/
bool ProfilerRunner::runWithGprof(const StringList& inCommand, const std::string& inExecutable)
{
#if defined(CHALET_WIN32)
	if (Output::showCommands())
		Output::printCommand(inCommand);

	WindowsTerminal::cleanup();
	Output::setShowCommandOverride(false);
#endif

	bool result = Process::runWithInput(inCommand);

#if defined(CHALET_WIN32)
	Output::setShowCommandOverride(true);
	WindowsTerminal::initialize();
#endif

	printExitedWithCode(result);

	if (!result)
		return false;

	auto executableName = String::getPathFilename(inExecutable);

	const auto& profiler = m_state.toolchain.profiler();
	const auto& buildDir = m_state.paths.buildOutputDir();
	const auto profStatsFile = fmt::format("{}/{}.stats", buildDir, executableName);
	// Output::msgProfilerStartedGprof(profStatsFile);
	// Output::lineBreak();

	std::string gmonOut{ "gmon.out" };

	if (!Process::runOutputToFile({ profiler, "-Q", "-b", inExecutable, gmonOut }, profStatsFile, PipeOption::StdOut))
	{
		Diagnostic::error("{} failed to save.", profStatsFile);
		return false;
	}

	Files::removeIfExists(gmonOut);

	// Output::lineBreak();
#if defined(CHALET_WIN32)
	if (m_state.info.launchProfiler())
	{
		Output::msgProfilerDoneAndLaunching(profStatsFile, std::string());
		Output::lineBreak();

		Files::sleep(1.0);

		auto statsFileForType = profStatsFile;
		Path::toWindows(statsFileForType);
		StringList cmd{ "type", statsFileForType };
		Process::runWithInput({ m_state.tools.commandPrompt(), "/c", String::join(cmd) });
	}
#else
	if (m_state.info.launchProfiler() && m_state.tools.bashAvailable())
	{
		Output::msgProfilerDoneAndLaunching(profStatsFile, std::string());
		Output::lineBreak();

		Files::sleep(1.0);

		StringList cmd{ "cat", profStatsFile };
		cmd.emplace_back("|");
		cmd.emplace_back("more");
		Process::runWithInput({ m_state.tools.bash(), "-c", String::join(cmd) });
	}
#endif
	else
	{
		Output::msgProfilerDone(profStatsFile);
		Output::lineBreak();
	}

	return true;
}

#if defined(CHALET_WIN32)
/*****************************************************************************/
// https://learn.microsoft.com/en-us/visualstudio/profiling/profile-apps-from-command-line?view=vs-2022
// Note: this was added midway through VS 2022's lifecycle (some time in 2023?)
//
bool ProfilerRunner::runWithVisualStudioDiagnostics(const StringList& inCommand, const std::string& inExecutable)
{
	auto& projectName = m_project.name();
	const auto& buildDir = m_state.paths.buildOutputDir();

	const auto& vsdiagnostics = m_state.toolchain.profiler();
	auto collectorPath = String::getPathFolder(vsdiagnostics);
	auto configFile = fmt::format("{}/AgentConfigs/CpuUsageWithCallCounts.json", collectorPath);
	if (!Files::pathExists(configFile))
	{
		configFile = fmt::format("{}/AgentConfigs/CpuUsageBase.json", collectorPath);
		if (!Files::pathExists(configFile))
		{
			Diagnostic::error("Failed to start diagnostic session with VSDiagnostics: Could not find a usable agent configuration.");
			return false;
		}
	}

	auto analysisFile = fmt::format("{}/{}.diagsession", buildDir, projectName);
	if (Files::pathExists(analysisFile))
	{
		if (!Files::removeRecursively(analysisFile))
			return false;
	}

	// We want to use a timestamp here, because if a session stays open, the folder path
	//   needs to be removed with elevated privalidges. Let the user handle it for now
	//
	time_t currentTimestamp = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
	const auto& outputDirectory = m_state.paths.outputDirectory();
	auto vsdiagnosticsScratchPath = fmt::format("{}/.vsdiagnostics", outputDirectory);
	auto scratchLocation = Files::getCanonicalPath(fmt::format("{}/session_{}", vsdiagnosticsScratchPath, currentTimestamp));
	if (Files::pathExists(scratchLocation))
	{
		if (!Files::removeRecursively(scratchLocation))
			return false;
	}

	Files::makeDirectory(vsdiagnosticsScratchPath);
	Files::makeDirectory(scratchLocation);

	// The VS Diagnostics Collector runs as an elevated service,
	//   but there's some issues with it getting the default temp directory,
	//   so we override it here with a path matching the current time.
	//   It gets very crashy and doesn't clean up stale sessions, nor can we clean up
	//   stale sessions due to the elevated path lock that the service uses
	//
	// https://developercommunity.visualstudio.com/t/cannot-run-performance-profiler/1325325
	//
	auto oldTemp1 = Environment::getString("TEMP");
	auto oldTemp2 = Environment::getString("TMP");

	Environment::set("TEMP", scratchLocation);
	Environment::set("TMP", scratchLocation);

	std::string sessionId{ "1" };

	// Start the session itself, so that when the actual process starts, we can attach it immediately
	bool result = Process::runMinimalOutput({
		vsdiagnostics,
		"start",
		sessionId,
		fmt::format("/loadConfig:{}", configFile),
	});

	if (result)
	{
		// attach the process, so we can still use stdin/stdout in our terminal environment
		result = Process::runWithInput(inCommand, [&](i32 pid) -> void {
			Process::runMinimalOutputWithoutWait({
				vsdiagnostics,
				"update",
				sessionId,
				fmt::format("/attach:{}", pid),
			});
		});

		// Stop the session. Annoyingly, this doesn't remove the scratch path lock if there was a previous failure
		Process::run({
			vsdiagnostics,
			"stop",
			sessionId,
			fmt::format("/output:{}", analysisFile),
		});

		printExitedWithCode(result);

		result = Files::pathExists(analysisFile);
	}
	else
	{

		Diagnostic::error("Failed to start VSDiagnostics for: {}", inExecutable);
	}

	Environment::set("TEMP", oldTemp1);
	Environment::set("TMP", oldTemp2);

	Files::removeRecursively(scratchLocation);

	if (Files::pathIsEmpty(vsdiagnosticsScratchPath))
		Files::remove(vsdiagnosticsScratchPath);

	return completeVisualStudioProfilingSession(inExecutable, analysisFile, result);
}

/*****************************************************************************/
// https://docs.microsoft.com/en-us/visualstudio/profiling/how-to-instrument-a-native-component-and-collect-timing-data?view=vs-2017
//
bool ProfilerRunner::runWithVisualStudioInstruments(const StringList& inCommand, const std::string& inExecutable)
{
	const auto& vsperfcmd = m_state.tools.vsperfcmd();
	chalet_assert(!vsperfcmd.empty(), "");
	if (vsperfcmd.empty())
		return false;

	const auto& vsinstruments = m_state.toolchain.profiler();
	const auto& buildDir = m_state.paths.buildOutputDir();
	auto executableName = String::getPathFilename(inExecutable);

	auto analysisFile = fmt::format("{}/{}.vsp", buildDir, executableName);
	if (Files::pathExists(analysisFile))
	{
		if (!Files::removeRecursively(analysisFile))
			return false;
	}

	// This returns false if the executable didn't change and the profiler *.instr.pdb already exists,
	//   so we don't care about the result
	Process::runMinimalOutput({
		vsinstruments,
		"/U",
		inExecutable,
	});

	// We need *.instr.pdb files for shared libraries as well
	for (auto& target : m_state.targets)
	{
		if (target->isSources())
		{
			auto& project = static_cast<SourceTarget&>(*target);
			if (project.isSharedLibrary())
			{
				auto file = m_state.paths.getTargetFilename(project);
				Process::runMinimalOutput({ vsinstruments, "/U", file });
			}
		}
	}

	// Start the trace service
	if (!Process::runMinimalOutput({
			vsperfcmd,
			"/start:trace",
			fmt::format("/output:{}", analysisFile),
		}))
	{
		Diagnostic::error("Failed to start trace: {}", analysisFile);
		return false;
	}

	// Run the command
	if (Output::showCommands())
		Output::printCommand(inCommand);

	WindowsTerminal::cleanup();
	Output::setShowCommandOverride(false);

	bool result = Process::runWithInput(inCommand);

	Output::setShowCommandOverride(true);
	WindowsTerminal::initialize();

	// Shut down the service
	if (!Process::runMinimalOutput({
			vsperfcmd,
			"/shutdown",
		}))
	{
		Diagnostic::error("Failed to shutdown trace: {}", analysisFile);
		return false;
	}

	printExitedWithCode(result);

	return completeVisualStudioProfilingSession(inExecutable, analysisFile, result);
}

/*****************************************************************************/
bool ProfilerRunner::completeVisualStudioProfilingSession(const std::string& inExecutable, const std::string& inAnalysisFile, const bool inResult)
{
	if (!inResult)
	{
		Diagnostic::error("Failed to run profiler for: {}", inExecutable);
		return false;
	}

	if (m_state.info.launchProfiler())
	{
		auto absAnalysisFile = Files::getAbsolutePath(inAnalysisFile);

		std::string devEnvDir;
		auto visualStudio = Files::which("devenv");
		if (visualStudio.empty())
		{
			devEnvDir = Environment::getString("DevEnvDir");
			visualStudio = fmt::format("{}\\devenv.exe", devEnvDir);
			if (devEnvDir.empty() || !Files::pathExists(visualStudio))
			{
				Diagnostic::error("Failed to launch in Visual Studio: {}", inAnalysisFile);
				return false;
			}
		}
		else
		{
			devEnvDir = String::getPathFolder(visualStudio);
		}

		Output::msgProfilerDoneAndLaunching(inAnalysisFile, "Visual Studio");
		Output::lineBreak();

		Files::sleep(1.0);

		Process::runMinimalOutput({ visualStudio, absAnalysisFile }, devEnvDir);
	}
	else
	{
		Output::msgProfilerDone(inAnalysisFile);
		Output::lineBreak();
	}

	return true;
}

#elif defined(CHALET_MACOS)
/*****************************************************************************/
bool ProfilerRunner::runWithInstruments(const StringList& inCommand, const std::string& inExecutable, const bool inUseXcTrace)
{
	// TOOD: profile could be defined elsewhere (maybe the cache json?)
	std::string profile = "Time Profiler";

	auto executableName = String::getPathFilename(inExecutable);

	const auto& buildDir = m_state.paths.buildOutputDir();
	auto instrumentsTrace = fmt::format("{}/{}.trace", buildDir, executableName);
	if (Files::pathExists(instrumentsTrace))
	{
		if (!Files::removeRecursively(instrumentsTrace))
			return false;
	}

	auto libPath = Environment::get("DYLD_FALLBACK_LIBRARY_PATH");
	auto frameworkPath = Environment::get("DYLD_FALLBACK_FRAMEWORK_PATH");

	bool result = true;

	// TODO: Could attach iPhone device here
	if (inUseXcTrace)
	{
		StringList cmd{ m_state.tools.xcrun(), "xctrace", "record", "--output", instrumentsTrace };

		cmd.emplace_back("--template");
		cmd.emplace_back(std::move(profile));

		// device

		cmd.emplace_back("--target-stdout");
		cmd.emplace_back("-");

		cmd.emplace_back("--target-stdin");
		cmd.emplace_back("-");

		cmd.emplace_back("--env");
		cmd.emplace_back(fmt::format("DYLD_FALLBACK_LIBRARY_PATH={}", libPath));

		cmd.emplace_back("--env");
		cmd.emplace_back(fmt::format("DYLD_FALLBACK_FRAMEWORK_PATH={}", frameworkPath));

		cmd.emplace_back("--launch");
		cmd.emplace_back("--");
		for (auto& arg : inCommand)
		{
			cmd.push_back(arg);
		}

		result = Process::runWithInput(cmd);
	}
	else
	{
		StringList cmd{ m_state.tools.instruments(), "-t", std::move(profile), "-D", instrumentsTrace };

		cmd.emplace_back("-e");
		cmd.emplace_back("DYLD_FALLBACK_LIBRARY_PATH");
		cmd.emplace_back(std::move(libPath));

		cmd.emplace_back("-e");
		cmd.emplace_back("DYLD_FALLBACK_FRAMEWORK_PATH");
		cmd.emplace_back(std::move(frameworkPath));

		for (auto& arg : inCommand)
		{
			cmd.push_back(arg);
		}

		Diagnostic::info("Running {} through instruments without output...", inExecutable);
		Output::lineBreak();

		result = Process::runWithInput(cmd);
	}

	printExitedWithCode(result);

	if (!result)
		return false;

	if (m_state.info.launchProfiler())
	{
		Output::msgProfilerDoneAndLaunching(instrumentsTrace, "Instruments");
		Output::lineBreak();

		Files::sleep(1.0);

		auto open = Files::which("open");
		Process::run({ std::move(open), instrumentsTrace });
	}
	else
	{
		Output::msgProfilerDone(instrumentsTrace);
		Output::lineBreak();
	}

	return true;
}

/*****************************************************************************/
bool ProfilerRunner::runWithSample(const StringList& inCommand, const std::string& inExecutable)
{
	auto executableName = String::getPathFilename(inExecutable);

	const auto& buildDir = m_state.paths.buildOutputDir();
	auto profStatsFile = fmt::format("{}/{}.stats", buildDir, executableName);

	bool sampleResult = true;
	auto onCreate = [&](i32 pid) -> void {
		u32 sampleDuration = 300;
		u32 samplingInterval = 1;
		Output::msgProfilerStartedSample(inExecutable, sampleDuration, samplingInterval);
		Output::lineBreak();

		sampleResult =
			Process::run({ m_state.tools.sample(), std::to_string(pid), std::to_string(sampleDuration), std::to_string(samplingInterval), "-wait", "-mayDie", "-file", profStatsFile }, PipeOption::Close);
	};

	bool result = Process::runWithInput(inCommand, std::move(onCreate));
	if (!sampleResult)
	{
		Diagnostic::error("Error running sample...");
		return false;
	}

	printExitedWithCode(result);

	if (!result)
		return false;

	if (m_state.info.launchProfiler() && m_state.tools.bashAvailable())
	{
		Output::msgProfilerDoneAndLaunching(profStatsFile, std::string());
		Output::lineBreak();

		Files::sleep(1.0);

		StringList cmd{ "cat", profStatsFile, "|", "more" };
		Process::runWithInput({ m_state.tools.bash(), "-c", String::join(cmd) });
	}
	else
	{
		Output::msgProfilerDone(profStatsFile);
		Output::lineBreak();
	}

	return true;
}

#endif
}
