/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Builder/BuildManager/ProfilerRunner.hpp"

#include "Libraries/Format.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Output.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
ProfilerRunner::ProfilerRunner(const BuildState& inState, const ProjectConfiguration& inProject, const bool inCleanOutput) :
	m_state(inState),
	m_project(inProject),
	m_cleanOutput(inCleanOutput)
{
}

/*****************************************************************************/
bool ProfilerRunner::run(const std::string& inExecutable, const std::string& inOutputFolder, const int inPid)
{
	auto& compilerConfig = m_state.compilers.getConfig(m_project.language());
#if defined(CHALET_LINUX) || defined(CHALET_WIN32)
	UNUSED(inPid);
	// at the moment, don't even try to run gprof on mac
	if (compilerConfig.isGcc() && !m_state.tools.gprof().empty())
	{
		Output::print(Color::Gray, "   Run task completed successfully. Profiling data for gprof has been written to gmon.out.");
		const auto profStatsFile = fmt::format("{}/profiler_analysis.stats", inOutputFolder);
		Output::msgProfilerStartedGprof(profStatsFile);

		if (!Commands::subprocessOutputToFile({ m_state.tools.gprof(), "-Q", "-b", inExecutable, "gmon.out" }, profStatsFile, PipeOption::StdOut, m_cleanOutput))
		{
			Diagnostic::errorAbort(fmt::format("{} failed to save.", profStatsFile));
			return false;
		}

		Output::msgProfilerDone(profStatsFile);
		Output::lineBreak();
	}
	else
#elif defined(CHALET_MACOS)
	UNUSED(inExecutable, inOutputFolder, inPid);
	if (compilerConfig.isAppleClang())
	{
		/*
			Notes:
				Nice resource on the topic of profiling in mac:
				https://gist.github.com/loderunner/36724cc9ee8db66db305

			sudo xcode-select -s /Library/Developer/CommandLineTools
			sudo xcode-select -s /Applications/Xcode.app/Contents/Developer

			'xcrun xctrace' should be the standard (from which Xcode version?)
			'instruments' was deprecated in favor of 'xcrun xctrace'

			CommandLineTools does not have access to instruments, or xcrun xctrace
			'sample' will need to be used instead if only CommandLineTools is selected
			both instruments and sample require the PID (get from subprocess somehow)

			... 🤡
		*/

		const auto& xcrun = m_state.tools.xcrun();
		const auto xctraceOutput = Commands::subprocessOutput({ xcrun, "xctrace" });
		const bool xctraceAvailable = !String::contains("unable to find utility", xctraceOutput);
		UNUSED(xctraceAvailable);

		// if (m_state.tools.xcodeVersionMajor() >= 12 || xctraceAvailable)
		// {
		// 	Diagnostic::errorAbort("call xcrun here");
		// 	return false;
		// }
		// else
		{
			const auto& instruments = m_state.tools.instruments();
			const auto instrumentsOutput = Commands::subprocessOutput({ instruments });
			const bool instrumentsAvailable = !String::contains("requires Xcode", instrumentsOutput);

			if (instrumentsAvailable)
			{
				auto instrumentsTrace = fmt::format("{}/profiler_analysis.trace", inOutputFolder);
				if (Commands::pathExists(instrumentsTrace))
				{
					if (!Commands::removeRecursively(instrumentsTrace, m_cleanOutput))
						return false;
				}

				std::string profile = "Time Profiler";
				if (Commands::pathExists(instrumentsTrace))
				{
					if (!Commands::removeRecursively(instrumentsTrace, m_cleanOutput))
						return false;
				}

				if (!Commands::subprocess({ instruments, "-t", std::move(profile), "-D", instrumentsTrace, "-p", std::to_string(inPid) }, m_cleanOutput))
					return false;

				Output::msgProfilerDoneInstruments(instrumentsTrace);
				Output::lineBreak();
				Commands::sleep(1.0);

				if (!Commands::subprocess({ "open", instrumentsTrace }))
					return false;
			}
			else
			{
				auto profStatsFile = fmt::format("{}/profiler_analysis.stats", inOutputFolder);
				uint sampleDuration = 300;
				uint samplingInterval = 1;
				Output::msgProfilerStartedSample(inExecutable, sampleDuration, samplingInterval);

				if (!Commands::subprocess({ m_state.tools.sample(), std::to_string(inPid), std::to_string(sampleDuration), std::to_string(samplingInterval), "-wait", "-mayDie", "-file", profStatsFile }, PipeOption::Close, m_cleanOutput))
					return false;

				Output::msgProfilerDone(profStatsFile);
				Output::lineBreak();
			}
		}
	}
	else
#endif
	{
		Diagnostic::errorAbort("Profiling is not been implemented on this compiler yet");
		return false;
	}

	return true;
}
}
