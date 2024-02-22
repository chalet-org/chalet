/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/CommandPool.hpp"

#include <csignal>

#include "Process/Environment.hpp"
#include "Process/SubProcessController.hpp"
#include "System/Files.hpp"
#include "System/SignalHandler.hpp"
#include "Terminal/Output.hpp"
#include "Terminal/Shell.hpp"
#include "Utility/List.hpp"
#include "Utility/Path.hpp"
#include "Utility/String.hpp"

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
	size_t refCount = 0;
	u32 index = 0;
	// u32 threads = 0;

	CommandPoolErrorCode errorCode = CommandPoolErrorCode::None;
	std::function<bool()> shutdownHandler;

	std::mutex mutex;

	std::vector<size_t> erroredOn;
};

static PoolState* state = nullptr;

/*****************************************************************************/
bool printCommand(std::string inText)
{
	std::lock_guard<std::mutex> lock(state->mutex);
	String::replaceAll(inText, '#', std::to_string(state->index));

	std::cout.write(inText.data(), inText.size());
	std::cout.put('\n');

	// if (state->index % state->threads == 1)
	std::cout.flush();

	++state->index;

	return true;
}

/*****************************************************************************/
#if defined(CHALET_WIN32)
bool executeCommandMsvc(size_t inIndex, StringList inCommand, std::string sourceFile, std::string dependencyFile)
{
	std::string output;

	ProcessOptions options;
	options.stdoutOption = PipeOption::Pipe;
	options.stderrOption = PipeOption::Pipe;
	options.onStdOut = [&sourceFile, &output](std::string inData) {
		output += std::move(inData);
	};
	options.onStdErr = options.onStdOut;

	bool result = true;
	if (SubProcessController::run(inCommand, options) != EXIT_SUCCESS)
		result = false;

	// String::replaceAll(output, "\r\n", "\n");
	// String::replaceAll(output, '\n', "\r\n");

	if (String::startsWith(sourceFile, output))
		String::replaceAll(output, fmt::format("{}\r\n", sourceFile), "");

	if (!output.empty())
	{
		std::lock_guard<std::mutex> lock(state->mutex);

		auto vcInstallDir = Environment::getString("VCINSTALLDIR");
		auto ucrtsdkDir = Environment::getString("UniversalCRTSdkDir");
		auto cwd = Files::getWorkingDirectory() + '\\';

		std::string toPrint;
		std::string dependencySearch("Note: including file: ");

		std::string dependencies;

		std::istringstream input(output);
		for (std::string line; std::getline(input, line);)
		{
			if (String::startsWith(dependencySearch, line))
			{
				auto file = line.substr(dependencySearch.size());
				auto firstNonSpace = file.find_first_not_of(' ');
				if (firstNonSpace > 0)
				{
					file = file.substr(firstNonSpace);
				}

				// Don't include system headers - if the toolchain version changes, we'll figure that out elsewhere
				//
				if (!vcInstallDir.empty() && String::startsWith(vcInstallDir, file))
					continue;

				if (!ucrtsdkDir.empty() && String::startsWith(ucrtsdkDir, file))
					continue;

				String::replaceAll(file, cwd, "");

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
				Files::createFileWithContents(dependencyFile, dependencies);
			}

			std::cout.write(toPrint.data(), toPrint.size());
		}
		else
		{
			if (state->errorCode == CommandPoolErrorCode::None)
				state->errorCode = CommandPoolErrorCode::BuildFailure;

			state->erroredOn.push_back(inIndex);

			auto error = Output::getAnsiStyle(Output::theme().error);
			auto reset = Output::getAnsiStyle(Output::theme().reset);
			auto cmdString = String::join(inCommand);

			auto failure = fmt::format("{}FAILED: {}{}\r\n", error, reset, cmdString);
			std::cout.write(failure.data(), failure.size());
			std::cout.write(toPrint.data(), toPrint.size());
		}
		std::cout.flush();
	}

	return result;
}
#endif

/*****************************************************************************/
bool executeCommand(size_t inIndex, StringList inCommand)
{
	std::string output;

	ProcessOptions options;
	options.stdoutOption = PipeOption::StdOut;
	options.stderrOption = PipeOption::Pipe;
	options.onStdErr = [&output](std::string inData) {
		output += std::move(inData);
	};

	bool result = true;
	if (SubProcessController::run(inCommand, options) != EXIT_SUCCESS)
		result = false;

	if (!output.empty())
	{
		std::lock_guard<std::mutex> lock(state->mutex);
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

			state->erroredOn.push_back(inIndex);

			auto error = Output::getAnsiStyle(Output::theme().error);
			auto reset = Output::getAnsiStyle(Output::theme().reset);
			auto cmdString = String::join(inCommand);

			auto failure = fmt::format("{}FAILED: {}{}{}", error, reset, cmdString, eol);
			std::cout.write(failure.data(), failure.size());
			std::cout.write(output.data(), output.size());
		}
		std::cout.flush();
	}

	return result;
}

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
CommandPool::CommandPool(const size_t inThreads) :
	m_threadPool(inThreads)
{
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
CommandPool::~CommandPool()
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
bool CommandPool::runAll(JobList& inJobs, Settings& inSettings)
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
bool CommandPool::run(const Job& inJob, const Settings& inSettings)
{
	auto&& [cmdColor, startIndex, total, quiet, showCommmands, keepGoing, msvcCommand] = inSettings;

#if !defined(CHALET_WIN32)
	UNUSED(msvcCommand);
#endif

	m_exceptionThrown.clear();
	state->errorCode = CommandPoolErrorCode::None;
	state->erroredOn.clear();
	m_quiet = quiet;

	state->shutdownHandler = [this]() -> bool {
		// if (state->errorCode != CommandPoolErrorCode::None)
		// 	return false;

		// this->m_threadPool.stop();
		// state->errorCode = CommandPoolErrorCode::Aborted;

		this->m_threadPool.stop();

		if (state->errorCode == CommandPoolErrorCode::None)
			state->errorCode = CommandPoolErrorCode::Aborted;

		return true;
	};

	Output::setQuietNonBuild(false);

	state->index = startIndex > 0 ? startIndex : 1;
	u32 totalCompiles = total;
	if (totalCompiles == 0)
	{
		totalCompiles = static_cast<u32>(inJob.list.size());
	}

	m_reset = Output::getAnsiStyle(Output::theme().reset);
	auto color = Output::getAnsiStyle(cmdColor);

	bool haltOnError = !keepGoing;

	// state->threads = inJob.threads > 0 ? inJob.threads : static_cast<u32>(m_threadPool.threads());
	if (totalCompiles <= 1 || inJob.threads == 1)
	{
		size_t index = 0;
		for (auto& it : inJob.list)
		{
			if (it.command.empty())
				continue;

			if (!printCommand(getPrintedText(fmt::format("{}{}", color, (showCommmands ? String::join(it.command) : it.output)),
					totalCompiles))
				&& haltOnError)
				break;

#if defined(CHALET_WIN32)
			if (msvcCommand)
			{
				if (!executeCommandMsvc(index, it.command, String::getPathFilename(it.reference), it.dependency) && haltOnError)
					break;
			}
			else
#endif
			{
				if (!executeCommand(index, it.command) && haltOnError)
					break;
			}

			++index;
		}
	}
	else
	{
		size_t index = 0;
		std::vector<std::future<bool>> threadResults;
		for (auto& it : inJob.list)
		{
			if (it.command.empty())
				continue;

			threadResults.emplace_back(m_threadPool.enqueue(
				printCommand,
				getPrintedText(fmt::format("{}{}", color, (showCommmands ? String::join(it.command) : it.output)),
					totalCompiles)));

#if defined(CHALET_WIN32)
			if (msvcCommand)
			{
				threadResults.emplace_back(m_threadPool.enqueue(executeCommandMsvc, index, it.command, String::getPathFilename(it.reference), it.dependency));
			}
			else
#endif
			{
				threadResults.emplace_back(m_threadPool.enqueue(executeCommand, index, it.command));
			}

			++index;
		}

		for (auto& tr : threadResults)
		{
			CHALET_TRY
			{
				if (!tr.get() && haltOnError)
				{
					CHALET_THROW(std::runtime_error("build error"));
				}
			}
			CHALET_CATCH(const std::future_error&)
			{}
			CHALET_CATCH(const std::exception& err)
			{
				if (state->errorCode != CommandPoolErrorCode::None && m_exceptionThrown.empty())
				{
					signalHandler(SIGTERM);
					if (String::equals("build error", err.what()))
					{
					}
					else
					{
						m_exceptionThrown = std::string(err.what());
						state->errorCode = CommandPoolErrorCode::BuildException;
					}
				}
			}
		}

		if (state->errorCode != CommandPoolErrorCode::None)
			threadResults.clear();
	}

	if (state->errorCode != CommandPoolErrorCode::None)
	{
		size_t index = 0;
		for (auto& it : inJob.list)
		{
			if (List::contains(state->erroredOn, index))
			{
				m_failures.push_back(it.reference);
			}

			++index;
		}
		// m_threadPool.stop();

		return onError();
	}

	cleanup();

	return true;
}

/*****************************************************************************/
const StringList& CommandPool::failures() const
{
	return m_failures;
}

/*****************************************************************************/
std::string CommandPool::getPrintedText(std::string inText, u32 inTotal)
{
	if (inTotal > 0)
		return fmt::format("{}   [#/{}] {}{}", m_reset, inTotal, inText, m_reset);
	else
		return fmt::format("{}   {}{}", m_reset, inText, m_reset);
}

/*****************************************************************************/
bool CommandPool::onError()
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
void CommandPool::cleanup()
{
	state->erroredOn.clear();
	state->shutdownHandler = nullptr;
	state->index = 0;

	Output::setQuietNonBuild(m_quiet);

	state->errorCode = CommandPoolErrorCode::None;
}
}
