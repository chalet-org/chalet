/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/CommandPoolAlt.hpp"

#if CHALET_ALT_COMMAND_POOL

// #include <csignal>

	#include "Process/Environment.hpp"
	#include "Process/SubProcessController.hpp"
	#include "System/Files.hpp"
	#include "System/SignalHandler.hpp"
	#include "Terminal/Output.hpp"
	#include "Terminal/Shell.hpp"
	#include "Utility/List.hpp"
	#include "Utility/Path.hpp"
	#include "Utility/String.hpp"

	#if defined(max)
		#undef max
	#endif

namespace chalet
{
/*****************************************************************************/
namespace
{
enum class CommandPoolErrorCode : u16
{
	None,
	Aborted,
	BuildFailure,
	BuildException,
};

struct PoolState
{
	#if defined(CHALET_WIN32)
	std::string vcInstallDir;
	std::string ucrtsdkDir;
	std::string cwd;
	std::string dependencySearch;
	#endif

	size_t refCount = 0;

	CommandPoolErrorCode errorCode = CommandPoolErrorCode::None;
	std::function<bool()> shutdownHandler;

	std::vector<size_t> erroredOn;
};

static PoolState* state = nullptr;

/*****************************************************************************/
void signalHandler(i32 inSignal)
{
	if (state == nullptr)
		return;

	if (inSignal != SIGTERM)
		state->errorCode = CommandPoolErrorCode::Aborted;

	state->shutdownHandler();
}
}

/*****************************************************************************/
CommandPoolAlt::CommandPoolAlt(const size_t inMaxJobs) :
	m_maxJobs(inMaxJobs)
{
	UNUSED(inMaxJobs);

	if (state == nullptr)
	{
		state = new PoolState();

		SignalHandler::add(SIGINT, signalHandler);
		SignalHandler::add(SIGTERM, signalHandler);
		SignalHandler::add(SIGABRT, signalHandler);
	}

	if (state->refCount < std::numeric_limits<size_t>::max())
		state->refCount++;
}

/*****************************************************************************/
CommandPoolAlt::~CommandPoolAlt()
{
	if (state != nullptr)
	{
		if (state->refCount > 0)
			state->refCount--;

		if (state->refCount == 0)
		{
			SignalHandler::remove(SIGINT, signalHandler);
			SignalHandler::remove(SIGTERM, signalHandler);
			SignalHandler::remove(SIGABRT, signalHandler);

			delete state;
			state = nullptr;
		}
	}
}

/*****************************************************************************/
bool CommandPoolAlt::runAll(JobList& inJobs, Settings& inSettings)
{
	inSettings.startIndex = 1;
	inSettings.total = 0;

	for (auto& job : inJobs)
	{
		if (job->list.empty())
			continue;

		inSettings.total += static_cast<u32>(job->list.size());
	}

	for (auto& job : inJobs)
	{
		if (job->list.empty())
			continue;

		if (!run(*job, inSettings))
			return false;

		inSettings.startIndex += static_cast<u32>(job->list.size());
		job.reset();
	}

	inJobs.clear();
	return true;
}

/*****************************************************************************/
bool CommandPoolAlt::run(const Job& inJob, const Settings& inSettings)
{
	auto&& [cmdColor, startIndex, total, quiet, showCommmands, keepGoing, msvcCommand] = inSettings;

	m_processes.clear();
	m_exceptionThrown.clear();
	state->errorCode = CommandPoolErrorCode::None;
	state->erroredOn.clear();
	m_quiet = quiet;

	#if defined(CHALET_WIN32)
	if (msvcCommand)
	{
		state->vcInstallDir = Environment::getString("VCINSTALLDIR");
		state->ucrtsdkDir = Environment::getString("UniversalCRTSdkDir");
		state->cwd = Files::getWorkingDirectory() + '\\';
		state->dependencySearch = "Note: including file: ";
	}
	#endif

	state->shutdownHandler = []() -> bool {
		// if (state->errorCode != CommandPoolErrorCode::None)
		// 	return false;

		// state->errorCode = CommandPoolErrorCode::Aborted;

		if (state->errorCode == CommandPoolErrorCode::None)
			state->errorCode = CommandPoolErrorCode::Aborted;

		return true;
	};

	Output::setQuietNonBuild(false);

	m_index = startIndex > 0 ? startIndex : 1;
	u32 totalCompiles = total;
	if (totalCompiles == 0)
	{
		totalCompiles = static_cast<u32>(inJob.list.size());
	}

	m_reset = Output::getAnsiStyle(Output::theme().reset);
	const auto& color = Output::getAnsiStyle(cmdColor);

	bool haltOnError = !keepGoing;

	{
		size_t jobCount = inJob.list.size();
		size_t maxJobs = jobCount < m_maxJobs ? jobCount : m_maxJobs;
		m_processes.resize(maxJobs);

		bool queuedAllJobs = false;
		size_t finishedJobs = 0;
		size_t index = 0;
		while (finishedJobs < inJob.list.size())
		{
			for (auto& process : m_processes)
			{
				if (process == nullptr && !queuedAllJobs)
				{
					process = std::make_unique<RunningProcess>();

					auto& job = inJob.list.at(index);
					printCommand(getPrintedText(fmt::format("{}{}", color, (showCommmands ? String::join(job.command) : job.output)), totalCompiles));

					process->command = &job.command;
					process->index = index;
	#if defined(CHALET_WIN32)
					if (msvcCommand)
					{
						process->reference = &job.reference;
						process->dependencyFile = &job.dependency;
						process->filterMsvc = true;
					}
	#endif
					if (!process->createRunningProcess())
					{
						state->errorCode = CommandPoolErrorCode::BuildFailure;
						break;
					}

					index++;
					if (index == inJob.list.size())
						queuedAllJobs = true;
				}

				if (process != nullptr)
				{
					if (process->pollState(m_buffer))
					{
						process->getResultAndPrintOutput(m_buffer);
						if (!process->result && haltOnError)
						{
							state->errorCode = CommandPoolErrorCode::BuildFailure;
							break;
						}

						process.reset();
						finishedJobs++;
					}
				}

				if (state->errorCode != CommandPoolErrorCode::None)
					break;
			}

			if (state->errorCode != CommandPoolErrorCode::None)
				break;
		}
	}

	if (state->errorCode != CommandPoolErrorCode::None)
	{
		for (auto& process : m_processes)
		{
			if (process != nullptr)
			{
				process->getResultAndPrintOutput(m_buffer);
				process->process.kill();
			}
		}

		size_t index = 0;
		for (auto& it : inJob.list)
		{
			if (List::contains(state->erroredOn, index))
			{
				m_failures.push_back(it.reference);
			}

			++index;
		}

		return onError();
	}

	cleanup();

	return true;
}

/*****************************************************************************/
const StringList& CommandPoolAlt::failures() const
{
	return m_failures;
}

/*****************************************************************************/
void CommandPoolAlt::printCommand(std::string text)
{
	std::cout.write(text.data(), text.size());
	std::cout.put('\n');

	// if (state->index % state->threads == 1)
	std::cout.flush();
}

/*****************************************************************************/
std::string CommandPoolAlt::getPrintedText(std::string inText, u32 inTotal)
{
	if (inTotal > 0)
	{
		auto result = fmt::format("{}   [{}/{}] {}{}", m_reset, m_index, inTotal, inText, m_reset);
		++m_index;
		return result;
	}
	else
	{
		return fmt::format("{}   {}{}", m_reset, inText, m_reset);
	}
}

/*****************************************************************************/
bool CommandPoolAlt::onError()
{
	if (state->errorCode == CommandPoolErrorCode::Aborted)
	{
		Output::msgCommandPoolError("Aborted by user.");
	}
	else if (state->errorCode == CommandPoolErrorCode::BuildException)
	{
		if (!m_exceptionThrown.empty())
			Output::msgCommandPoolError(m_exceptionThrown);

		std::string term("Terminated running processes.");
		std::cout.write(term.data(), term.size());
	}

	cleanup();

	return false;
}

/*****************************************************************************/
void CommandPoolAlt::cleanup()
{
	m_processes.clear();
	state->erroredOn.clear();
	state->shutdownHandler = nullptr;
	m_index = 0;

	Output::setQuietNonBuild(m_quiet);

	state->errorCode = CommandPoolErrorCode::None;
}

/*****************************************************************************/
bool CommandPoolAlt::RunningProcess::createRunningProcess()
{
	options.waitForResult = false;
	options.stdoutOption = PipeOption::Pipe;
	options.stderrOption = PipeOption::Pipe;
	options.onStdOut = [this](std::string inData) {
		output += std::move(inData);
	};
	options.onStdErr = options.onStdOut;

	return SubProcessController::create(this->process, *command, options);
}

/*****************************************************************************/
bool CommandPoolAlt::RunningProcess::pollState(SubProcess::OutputBuffer& buffer)
{
	#if defined(CHALET_WIN32)
	auto bytesRead = process.getInitialReadValue();
	if (!process.killed() && process.readOnce(FileNo::StdOut, buffer, bytesRead))
	{
		options.onStdOut(std::string(buffer.data(), bytesRead));
	}
	#else
	UNUSED(buffer);
	#endif

	exitCode = process.pollState();
	return exitCode > -1;
}

/*****************************************************************************/
i32 CommandPoolAlt::RunningProcess::getLastExitCode()
{
	return SubProcessController::getLastExitCodeFromProcess(process, true);
}

/*****************************************************************************/
void CommandPoolAlt::RunningProcess::getResultAndPrintOutput(SubProcess::OutputBuffer& buffer)
{
	updateHandle(buffer, FileNo::StdOut, options.onStdOut);
	updateHandle(buffer, FileNo::StdErr, options.onStdErr);

	process.close();

	result = exitCode == EXIT_SUCCESS;
	if (!result)
	{
		state->erroredOn.push_back(index);
	}
	#if defined(CHALET_WIN32)
	if (filterMsvc)
		printMsvcOutput();
	else
	#endif
		printOutput();
}

/*****************************************************************************/
void CommandPoolAlt::RunningProcess::printOutput()
{
	if (!output.empty())
	{
		auto eol = String::eol();
		if (Shell::isMicrosoftTerminalOrWindowsBash())
		{
			String::replaceAll(output, '\n', eol);
		}

		if (result)
		{
			// Warnings
			std::cout.write(output.data(), output.size());
		}
		else
		{
			if (state->errorCode == CommandPoolErrorCode::None)
				state->errorCode = CommandPoolErrorCode::BuildFailure;

			const auto& error = Output::getAnsiStyle(Output::theme().error);
			const auto& reset = Output::getAnsiStyle(Output::theme().reset);
			auto cmdString = String::join(*command);

			auto failure = fmt::format("{}FAILED: {}{}{}", error, reset, cmdString, eol);
			std::cout.write(failure.data(), failure.size());
			std::cout.write(output.data(), output.size());
		}
		std::cout.flush();
	}
	output.clear();
}

	#if defined(CHALET_WIN32)
void CommandPoolAlt::RunningProcess::printMsvcOutput()
{
	// String::replaceAll(process.output, "\r\n", "\n");
	// String::replaceAll(process.output, '\n', "\r\n");

	auto sourceFile = String::getPathFilename(*reference);
	if (String::startsWith(sourceFile, output))
		String::replaceAll(output, fmt::format("{}\r\n", sourceFile), "");

	if (!output.empty())
	{
		std::string toPrint;
		std::string dependencies;

		std::istringstream input(output);
		for (std::string line; std::getline(input, line);)
		{
			if (String::startsWith(state->dependencySearch, line))
			{
				auto file = line.substr(state->dependencySearch.size());
				auto firstNonSpace = file.find_first_not_of(' ');
				if (firstNonSpace > 0)
				{
					file = file.substr(firstNonSpace);
				}

				// Don't include system headers - if the toolchain version changes, we'll figure that out elsewhere
				//
				if (!state->vcInstallDir.empty() && String::startsWith(state->vcInstallDir, file))
					continue;

				if (!state->ucrtsdkDir.empty() && String::startsWith(state->ucrtsdkDir, file))
					continue;

				String::replaceAll(file, state->cwd, "");

				if (String::endsWith('\n', file))
					file.pop_back();

				if (String::endsWith('\r', file))
					file.pop_back();

				// When the dependencies get read, we'll look for this
				file += ':';
				file += '\n';

				dependencies += std::move(file);
			}
			else
			{
				toPrint += line + "\r\n";
			}
		}

		if (result)
		{
			if (!dependencies.empty())
			{
				Path::toUnix(dependencies);
				Files::createFileWithContents(*dependencyFile, dependencies);
			}

			std::cout.write(toPrint.data(), toPrint.size());
		}
		else
		{
			if (state->errorCode == CommandPoolErrorCode::None)
				state->errorCode = CommandPoolErrorCode::BuildFailure;

			const auto& error = Output::getAnsiStyle(Output::theme().error);
			const auto& reset = Output::getAnsiStyle(Output::theme().reset);
			auto cmdString = String::join(*command);

			auto failure = fmt::format("{}FAILED: {}{}\r\n", error, reset, cmdString);
			std::cout.write(failure.data(), failure.size());
			std::cout.write(toPrint.data(), toPrint.size());
		}
		std::cout.flush();
	}
}
	#endif
/*****************************************************************************/
void CommandPoolAlt::RunningProcess::updateHandle(SubProcess::OutputBuffer& buffer, const SubProcess::HandleInput& inFileNo, const ProcessOptions::PipeFunc& onRead)
{
	auto bytesRead = process.getInitialReadValue();
	while (true)
	{
		if (process.killed())
			break;

		if (process.readOnce(inFileNo, buffer, bytesRead))
		{
			if (onRead != nullptr)
				onRead(std::string(buffer.data(), bytesRead));
		}
		else
			break;
	}
}

}
#endif
