/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Builder/ProfilerRunner.hpp"

#include "Libraries/Format.hpp"
#include "State/AncillaryTools.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Output.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
ProfilerRunner::ProfilerRunner(BuildState& inState, const ProjectTarget& inProject, const bool inCleanOutput) :
	m_state(inState),
	m_project(inProject),
	m_cleanOutput(inCleanOutput)
{
}

/*****************************************************************************/
bool ProfilerRunner::run(const StringList& inCommand, const std::string& inExecutable, const std::string& inOutputFolder)
{

	auto& compilerConfig = m_state.toolchain.getConfig(m_project.language());
	if (compilerConfig.isGcc() && !m_state.ancillaryTools.gprof().empty())
	{
		return ProfilerRunner::runWithGprof(inCommand, inExecutable, inOutputFolder);
	}
#if defined(CHALET_MACOS)
	else if (compilerConfig.isAppleClang())
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
			'sample' will need to be used instead if only CommandLineTools is selected
			sample require the PID (get from subprocess somehow), while both flavors of making an
			Instruments trace can be passed the commands directly

			... 🤡
		*/

		const auto xctraceOutput = Commands::subprocessOutput({ m_state.ancillaryTools.xcrun(), "xctrace" });
		const bool xctraceAvailable = !String::contains("unable to find utility", xctraceOutput);
		const bool useXcTrace = m_state.ancillaryTools.xcodeVersionMajor() >= 12 || xctraceAvailable;

		const auto instrumentsOutput = Commands::subprocessOutput({ m_state.ancillaryTools.instruments() });
		const bool instrumentsAvailable = !String::contains("requires Xcode", instrumentsOutput);

		if (xctraceAvailable || instrumentsAvailable)
		{
			return runWithInstruments(inCommand, inExecutable, inOutputFolder, useXcTrace);
		}
		else
		{
			return runWithSample(inCommand, inExecutable, inOutputFolder);
		}
	}
#endif
	else
	{
		Diagnostic::error("Profiling is not been implemented on this compiler yet");
		return false;
	}
}

/*****************************************************************************/
bool ProfilerRunner::runWithGprof(const StringList& inCommand, const std::string& inExecutable, const std::string& inOutputFolder)
{
	const bool result = Commands::subprocess(inCommand, m_cleanOutput);

	Output::print(Color::Gray, "   Run task completed successfully. Profiling data for gprof has been written to gmon.out.");

	const auto profStatsFile = fmt::format("{}/profiler_analysis.stats", inOutputFolder);
	Output::msgProfilerStartedGprof(profStatsFile);

	if (!Commands::subprocessOutputToFile({ m_state.ancillaryTools.gprof(), "-Q", "-b", inExecutable, "gmon.out" }, profStatsFile, PipeOption::StdOut, m_cleanOutput))
	{
		Diagnostic::error(fmt::format("{} failed to save.", profStatsFile));
		return false;
	}

	Output::msgProfilerDone(profStatsFile);
	Output::lineBreak();

	return result;
}

#if defined(CHALET_MACOS)

/*****************************************************************************/
bool ProfilerRunner::runWithInstruments(const StringList& inCommand, const std::string& inExecutable, const std::string& inOutputFolder, const bool inUseXcTrace)
{
	// TOOD: profile could be defined elsewhere (maybe the cache json?)
	std::string profile = "Time Profiler";

	auto instrumentsTrace = fmt::format("{}/profiler_analysis.trace", inOutputFolder);
	if (Commands::pathExists(instrumentsTrace))
	{
		if (!Commands::removeRecursively(instrumentsTrace, m_cleanOutput))
			return false;
	}

	// TODO: Could attach iPhone device here
	if (inUseXcTrace)
	{
		StringList cmd{ m_state.ancillaryTools.xcrun(), "xctrace", "record", "--output", instrumentsTrace };

		cmd.push_back("--template");
		cmd.push_back(std::move(profile));

		// device

		cmd.push_back("--target-stdout");
		cmd.push_back("-");

		cmd.push_back("--target-stdin");
		cmd.push_back("-");

		cmd.push_back("--launch");
		cmd.push_back("--");
		for (auto& arg : inCommand)
		{
			cmd.push_back(arg);
		}

		if (!Commands::subprocess(cmd, m_cleanOutput))
			return false;
	}
	else
	{
		StringList cmd{ m_state.ancillaryTools.instruments(), "-t", std::move(profile), "-D", instrumentsTrace };
		for (auto& arg : inCommand)
		{
			cmd.push_back(arg);
		}

		Output::print(Color::Gray, fmt::format("   Running {} through instruments without output...", inExecutable));
		Output::lineBreak();

		if (!Commands::subprocess(cmd, m_cleanOutput))
			return false;
	}

	Output::lineBreak();
	Output::msgProfilerDoneInstruments(instrumentsTrace);
	Output::lineBreak();

	Commands::sleep(1.0);

	return Commands::subprocess({ "open", instrumentsTrace });
}

/*****************************************************************************/
bool ProfilerRunner::runWithSample(const StringList& inCommand, const std::string& inExecutable, const std::string& inOutputFolder)
{
	uint sampleDuration = 300;
	uint samplingInterval = 1;

	auto profStatsFile = fmt::format("{}/profiler_analysis.stats", inOutputFolder);

	bool sampleResult = false;
	auto onCreate = [&](int pid) -> void {
		Output::msgProfilerStartedSample(inExecutable, sampleDuration, samplingInterval);

		sampleResult = Commands::subprocess({ m_state.ancillaryTools.sample(), std::to_string(pid), std::to_string(sampleDuration), std::to_string(samplingInterval), "-wait", "-mayDie", "-file", profStatsFile }, PipeOption::Close, m_cleanOutput);
	};

	bool result = Commands::subprocess(inCommand, std::move(onCreate), m_cleanOutput);
	if (!sampleResult)
		return false;

	Output::msgProfilerDone(profStatsFile);
	Output::lineBreak();

	return result;
}

#endif
}
