/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Builder/ProfilerRunner.hpp"

#include "Compile/Environment/ICompileEnvironment.hpp"
#include "Core/CommandLineInputs.hpp"
#include "Process/ProcessController.hpp"
#include "State/AncillaryTools.hpp"
#include "State/BuildPaths.hpp"
#include "State/BuildState.hpp"
#include "State/CompilerTools.hpp"
#include "State/Target/SourceTarget.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Environment.hpp"
#include "Terminal/Output.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
ProfilerRunner::ProfilerRunner(const CommandLineInputs& inInputs, BuildState& inState, const SourceTarget& inProject) :
	m_inputs(inInputs),
	m_state(inState),
	m_project(inProject)
{
}

/*****************************************************************************/
bool ProfilerRunner::run(const StringList& inCommand, const std::string& inExecutable)
{
	if (!m_state.toolchain.profiler().empty() && m_state.toolchain.isProfilerGprof())
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
			const auto xctraceOutput = Commands::subprocessOutput({ m_state.tools.xcrun(), "xctrace" });
			xctraceAvailable = !String::contains("unable to find utility", xctraceOutput);
		}
		const bool useXcTrace = m_state.tools.xcodeVersionMajor() >= 12 || xctraceAvailable;

		bool instrumentsAvailable = m_state.tools.xcodeVersionMajor() <= 12;
		if (instrumentsAvailable && !m_state.tools.instruments().empty())
		{
			const auto instrumentsOutput = Commands::subprocessOutput({ m_state.tools.instruments() });
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
	if (!m_state.toolchain.profiler().empty() && m_state.environment->isMsvc() && m_state.toolchain.isProfilerVSInstruments())
	{
		return runWithVisualStudioInstruments(inCommand, inExecutable);
	}
#endif

	// Not supported - This should be validated in BuildState::validate()
	return false;
}

/*****************************************************************************/
bool ProfilerRunner::runWithGprof(const StringList& inCommand, const std::string& inExecutable)
{
	const bool result = Commands::subprocessWithInput(inCommand);

	auto outFile = m_state.paths.getTargetFilename(m_project);
	m_inputs.clearWorkingDirectory(outFile);

	auto message = fmt::format("{} exited with code: {}", outFile, ProcessController::getLastExitCode());

	// Output::lineBreak();
	Output::printSeparator();
	Output::print(result ? Output::theme().info : Output::theme().error, message);
	Output::lineBreak();

	Diagnostic::info("Run task completed successfully. Profiling data for gprof has been written to gmon.out.");

	const auto& buildDir = m_state.paths.buildOutputDir();
	const auto profStatsFile = fmt::format("{}/profiler_analysis.stats", buildDir);
	Output::msgProfilerStartedGprof(profStatsFile);
	Output::lineBreak();

	if (!Commands::subprocessOutputToFile({ m_state.toolchain.profiler(), "-Q", "-b", inExecutable, "gmon.out" }, profStatsFile, PipeOption::StdOut))
	{
		Diagnostic::error("{} failed to save.", profStatsFile);
		return false;
	}

	// Output::lineBreak();
	Output::msgProfilerDone(profStatsFile);

	return result;
}

#if defined(CHALET_WIN32)
/*****************************************************************************/
bool ProfilerRunner::runWithVisualStudioInstruments(const StringList& inCommand, const std::string& inExecutable)
{
	// https://docs.microsoft.com/en-us/visualstudio/profiling/how-to-instrument-a-native-component-and-collect-timing-data?view=vs-2017
	//

	const auto& vsperfcmd = m_state.tools.vsperfcmd();
	chalet_assert(!vsperfcmd.empty(), "");
	if (vsperfcmd.empty())
		return false;

	const auto& buildDir = m_state.paths.buildOutputDir();
	auto analysisFile = fmt::format("{}/profiler_analysis.vsp", buildDir);
	if (Commands::pathExists(analysisFile))
	{
		if (!Commands::removeRecursively(analysisFile))
			return false;
	}

	/////////////

	// This returns false if the executable didn't change and the profiler *.instr.pdb already exists,
	//   so we don't care about the result
	Commands::subprocessNoOutput({
		m_state.toolchain.profiler(),
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
				Commands::subprocessNoOutput({ m_state.toolchain.profiler(), "/U", file });
			}
		}
	}

	/////////////

	// Start the trace service
	if (!Commands::subprocessNoOutput({
			vsperfcmd,
			"/start:trace",
			fmt::format("/output:{}", analysisFile),
		}))
	{
		Diagnostic::error("Failed to start trace: {}", analysisFile);
		return false;
	}

	// Run the command
	bool result = Commands::subprocessWithInput(inCommand);

	// Shut down the service
	result &= Commands::subprocessNoOutput({
		vsperfcmd,
		"/shutdown",
	});

	/////////////

	if (!result)
	{
		Diagnostic::error("Failed to save: {}", analysisFile);
		return false;
	}

	auto absAnalysisFile = Commands::getAbsolutePath(analysisFile);

	auto devEnvDir = Environment::getAsString("DevEnvDir");
	auto visualStudio = fmt::format("{}\\devenv.exe", devEnvDir);
	if (devEnvDir.empty() || !Commands::pathExists(visualStudio))
	{
		Diagnostic::error("Failed to launch in Visual Studio: {}", analysisFile);
		return false;
	}

	Output::printSeparator();
	Output::msgProfilerDoneAndLaunching(analysisFile, "Visual Studio");
	Output::lineBreak();

	Commands::sleep(1.0);

	Commands::subprocessNoOutput({ visualStudio, absAnalysisFile }, devEnvDir);

	return true;
}

#elif defined(CHALET_MACOS)
/*****************************************************************************/
bool ProfilerRunner::runWithInstruments(const StringList& inCommand, const std::string& inExecutable, const bool inUseXcTrace)
{
	// TOOD: profile could be defined elsewhere (maybe the cache json?)
	std::string profile = "Time Profiler";

	const auto& buildDir = m_state.paths.buildOutputDir();
	auto instrumentsTrace = fmt::format("{}/profiler_analysis.trace", buildDir);
	if (Commands::pathExists(instrumentsTrace))
	{
		if (!Commands::removeRecursively(instrumentsTrace))
			return false;
	}

	auto libPath = Environment::get("DYLD_FALLBACK_LIBRARY_PATH");
	auto frameworkPath = Environment::get("DYLD_FALLBACK_FRAMEWORK_PATH");

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

		if (!Commands::subprocessWithInput(cmd))
			return false;
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

		if (!Commands::subprocessWithInput(cmd))
			return false;
	}

	Output::printSeparator();
	Output::msgProfilerDoneAndLaunching(instrumentsTrace, "Instruments");
	Output::lineBreak();

	Commands::sleep(1.0);

	auto open = Commands::which("open");
	return Commands::subprocess({ std::move(open), instrumentsTrace });
}

/*****************************************************************************/
bool ProfilerRunner::runWithSample(const StringList& inCommand, const std::string& inExecutable)
{
	const auto& buildDir = m_state.paths.buildOutputDir();
	auto profStatsFile = fmt::format("{}/profiler_analysis.stats", buildDir);

	bool sampleResult = true;
	auto onCreate = [&](int pid) -> void {
		uint sampleDuration = 300;
		uint samplingInterval = 1;
		Output::msgProfilerStartedSample(inExecutable, sampleDuration, samplingInterval);
		Output::lineBreak();

		sampleResult =
			Commands::subprocess({ m_state.tools.sample(), std::to_string(pid), std::to_string(sampleDuration), std::to_string(samplingInterval), "-wait", "-mayDie", "-file", profStatsFile }, PipeOption::Close);
	};

	bool result = Commands::subprocessWithInput(inCommand, std::move(onCreate));
	if (!sampleResult)
		Diagnostic::error("Error running sample...");

	Output::lineBreak();
	Output::msgProfilerDone(profStatsFile);
	Output::lineBreak();

	return result;
}

#endif
}
