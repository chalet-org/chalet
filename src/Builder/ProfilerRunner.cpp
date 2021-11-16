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

			... 🤡
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
	if (m_state.environment->isGcc() && !m_state.toolchain.profiler().empty())
	{
		if (m_state.toolchain.isProfilerGprof())
		{
			return ProfilerRunner::runWithGprof(inCommand, inExecutable);
		}
	}

	Diagnostic::error("A profiler was either not found, or profiling on this toolchain is not yet supported.");
	return false;
}

/*****************************************************************************/
bool ProfilerRunner::runWithGprof(const StringList& inCommand, const std::string& inExecutable)
{
	const bool result = Commands::subprocess(inCommand);

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

#if defined(CHALET_MACOS)

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

		cmd.emplace_back("--launch");
		cmd.emplace_back("--");
		for (auto& arg : inCommand)
		{
			cmd.push_back(arg);
		}

		if (!Commands::subprocess(cmd))
			return false;
	}
	else
	{
		StringList cmd{ m_state.tools.instruments(), "-t", std::move(profile), "-D", instrumentsTrace };
		for (auto& arg : inCommand)
		{
			cmd.push_back(arg);
		}

		Diagnostic::info("Running {} through instruments without output...", inExecutable);
		Output::lineBreak();

		if (!Commands::subprocess(cmd))
			return false;
	}

	Output::printSeparator();
	Output::msgProfilerDoneInstruments(instrumentsTrace);
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

	bool result = Commands::subprocess(inCommand, std::move(onCreate));
	if (!sampleResult)
		Diagnostic::error("Error running sample...");

	Output::lineBreak();
	Output::msgProfilerDone(profStatsFile);
	Output::lineBreak();

	return result;
}

#endif
}
