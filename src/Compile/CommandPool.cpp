/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/CommandPool.hpp"

#include <csignal>

#include "Process/ProcessController.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Environment.hpp"
#include "Terminal/Output.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
namespace
{
enum class CommandPoolErrorCode : ushort
{
	None,
	Aborted,
	BuildFailure,
	BuildException,
};

static std::mutex s_mutex;
static struct
{
	std::atomic<uint> index = 0;
	// std::atomic<CommandPoolErrorCode> errorCode = CommandPoolErrorCode::None;
	CommandPoolErrorCode errorCode = CommandPoolErrorCode::None;
	std::function<bool()> shutdownHandler;
} state;

/*****************************************************************************/
bool printCommand(std::string text)
{
	String::replaceAll(text, '#', std::to_string(state.index));

	{
		std::lock_guard<std::mutex> lock(s_mutex);
		std::cout << text << std::endl;
	}

	++state.index;

	return true;
}

/*****************************************************************************/
#if defined(CHALET_WIN32)
bool executeCommandMsvc(StringList command, std::string sourceFile)
{
	std::string output;
	auto onOutput = [&sourceFile, &output](std::string inData) {
		if (!sourceFile.empty() && String::startsWith(sourceFile, inData))
			return;

		output += std::move(inData);
	};

	ProcessOptions options;
	options.stdoutOption = PipeOption::Pipe;
	options.stderrOption = PipeOption::Pipe;
	options.onStdOut = onOutput;
	options.onStdErr = onOutput;

	bool result = true;
	if (ProcessController::run(command, options) != EXIT_SUCCESS)
		result = false;

	if (!output.empty())
	{
		std::lock_guard<std::mutex> lock(s_mutex);
		String::replaceAll(output, '\n', "\r\n");

		if (result)
		{
			std::cout << output << std::flush;
		}
		else
		{
			state.errorCode = CommandPoolErrorCode::BuildFailure;

			auto error = Output::getAnsiStyle(Output::theme().error);
			auto reset = Output::getAnsiStyle(Color::Reset);
			auto cmdString = String::join(command);

			std::cout << fmt::format("{}FAILED: {}{}\r\n", error, reset, cmdString) << output << std::flush;
		}
	}

	return result;
}
#endif

/*****************************************************************************/
bool executeCommandCarriageReturn(StringList command)
{
	ProcessOptions options;
	static auto onStdOut = [](std::string inData) {
		String::replaceAll(inData, '\n', "\r\n");
		std::cout << inData << std::flush;
	};

	std::string errorOutput;
	auto onStdErr = [&errorOutput](std::string inData) {
		errorOutput += std::move(inData);
	};
	options.stdoutOption = PipeOption::Pipe;
	options.stderrOption = PipeOption::Pipe;
	options.onStdOut = onStdOut;
	options.onStdErr = onStdErr;

	bool result = true;
	if (ProcessController::run(command, options) != EXIT_SUCCESS)
		result = false;

	if (!errorOutput.empty())
	{
		std::lock_guard<std::mutex> lock(s_mutex);
		String::replaceAll(errorOutput, '\n', "\r\n");

		if (result)
		{
			// Warnings
			std::cout << errorOutput << std::flush;
		}
		else
		{
			state.errorCode = CommandPoolErrorCode::BuildFailure;

			auto error = Output::getAnsiStyle(Output::theme().error);
			auto reset = Output::getAnsiStyle(Color::Reset);
			auto cmdString = String::join(command);

			std::cout << fmt::format("{}FAILED: {}{}\r\n", error, reset, cmdString) << errorOutput << std::flush;
		}
	}

	return result;
}

/*****************************************************************************/
bool executeCommand(StringList command)
{
	ProcessOptions options;
	static auto onStdOut = [](std::string inData) {
		std::cout << inData << std::flush;
	};

	std::string errorOutput;
	auto onStdErr = [&errorOutput](std::string inData) {
		errorOutput += std::move(inData);
	};
	options.stdoutOption = PipeOption::Pipe;
	options.stderrOption = PipeOption::Pipe;
	options.onStdOut = onStdOut;
	options.onStdErr = onStdErr;

	bool result = true;
	if (ProcessController::run(command, options) != EXIT_SUCCESS)
		result = false;

	if (!errorOutput.empty())
	{
		std::lock_guard<std::mutex> lock(s_mutex);

		if (result)
		{
			// Warnings
			std::cout << errorOutput << std::flush;
		}
		else
		{
			state.errorCode = CommandPoolErrorCode::BuildFailure;

			auto error = Output::getAnsiStyle(Output::theme().error);
			auto reset = Output::getAnsiStyle(Color::Reset);
			auto cmdString = String::join(command);

			std::cout << fmt::format("{}FAILED: {}{}\n", error, reset, cmdString) << errorOutput << std::flush;
		}
	}

	return result;
}

/*****************************************************************************/
void signalHandler(int inSignal)
{
	if (state.shutdownHandler())
	{
		if (inSignal == SIGTERM)
		{
			// might result in a segfault, but if a SIGTERM has been sent, we really want to halt anyway
			ProcessController::haltAll(SigNum::Terminate);
		}
	}
}
}

/*****************************************************************************/
CommandPool::CommandPool(const std::size_t inThreads) :
	m_threadPool(inThreads)
{
}

/*****************************************************************************/
bool CommandPool::runAll(JobList&& inJobs, Settings& inSettings)
{
	inSettings.startIndex = 1;
	inSettings.total = 0;

	for (auto& job : inJobs)
	{
		inSettings.total += static_cast<uint>(job->list.size());
	}

	for (auto& job : inJobs)
	{
		if (!run(*job, inSettings))
			return false;

		inSettings.startIndex += static_cast<uint>(job->list.size());
		job.reset();
	}

	inJobs.clear();
	return true;
}

/*****************************************************************************/
bool CommandPool::run(const Job& inJob, const Settings& inSettings)
{
	auto&& [cmdColor, startIndex, total, quiet, showCommmands, msvcCommand] = inSettings;

	::signal(SIGINT, signalHandler);
	::signal(SIGTERM, signalHandler);
	::signal(SIGABRT, signalHandler);

#if !defined(CHALET_WIN32)
	UNUSED(msvcCommand);
#endif

	m_exceptionThrown.clear();
	state.errorCode = CommandPoolErrorCode::None;
	m_quiet = quiet;

	state.shutdownHandler = [this]() -> bool {
		if (state.errorCode != CommandPoolErrorCode::None)
			return false;

		this->m_threadPool.stop();
		state.errorCode = CommandPoolErrorCode::Aborted;
		return true;
	};

	Output::setQuietNonBuild(false);

	auto& executeCommandFunc = Environment::isMicrosoftTerminalOrWindowsBash() ?
		  executeCommandCarriageReturn :
		  executeCommand;

	state.index = startIndex > 0 ? startIndex : 1;
	uint totalCompiles = total;
	if (totalCompiles == 0)
	{
		totalCompiles = static_cast<uint>(inJob.list.size());
	}

	m_reset = Output::getAnsiStyle(Color::Reset);
	auto color = Output::getAnsiStyle(cmdColor);

	if (totalCompiles <= 1 || inJob.threads == 1)
	{
		for (auto& it : inJob.list)
		{
			if (it.command.empty())
				continue;

			if (!printCommand(getPrintedText(fmt::format("{}{}", color, (showCommmands ? String::join(it.command) : it.output)),
					totalCompiles)))
				return onError();

#if defined(CHALET_WIN32)
			if (msvcCommand)
			{
				if (!executeCommandMsvc(it.command, it.outputReplace))
					return onError();
			}
			else
#endif
			{
				if (!executeCommandFunc(it.command))
					return onError();
			}
		}

		if (state.errorCode != CommandPoolErrorCode::None)
			return onError();
	}
	else
	{
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
				threadResults.emplace_back(m_threadPool.enqueue(executeCommandMsvc, it.command, it.outputReplace));
			}
			else
#endif
			{
				threadResults.emplace_back(m_threadPool.enqueue(executeCommandFunc, it.command));
			}
		}

		for (auto& tr : threadResults)
		{
			CHALET_TRY
			{
				if (!tr.get())
				{
					CHALET_THROW(std::runtime_error("build error"));
				}
			}
			CHALET_CATCH(const std::exception& err)
			{
				if (state.errorCode != CommandPoolErrorCode::None && m_exceptionThrown.empty())
				{
					signalHandler(SIGTERM);
					if (String::equals("build error", err.what()))
					{}
					else
					{
						m_exceptionThrown = std::string(err.what());
						state.errorCode = CommandPoolErrorCode::BuildException;
					}
				}
			}
		}

		if (state.errorCode != CommandPoolErrorCode::None)
		{
			// m_threadPool.stop();
			threadResults.clear();

			return onError();
		}
	}

	cleanup();

	return true;
}

/*****************************************************************************/
std::string CommandPool::getPrintedText(std::string inText, uint inTotal)
{
	if (inTotal > 0)
		return fmt::format("{}   [#/{}] {}{}", m_reset, inTotal, inText, m_reset);
	else
		return fmt::format("{}   {}{}", m_reset, inText, m_reset);
}

/*****************************************************************************/
bool CommandPool::onError()
{
	if (state.errorCode == CommandPoolErrorCode::Aborted)
	{
		Output::msgCommandPoolError("Aborted by user.");
	}
	else if (state.errorCode == CommandPoolErrorCode::BuildException)
	{
		Output::msgCommandPoolError(m_exceptionThrown);
		std::cout << "Terminated running processes." << std::endl;
	}

	cleanup();

	Output::lineBreak();

	return false;
}

/*****************************************************************************/
void CommandPool::cleanup()
{
	state.shutdownHandler = nullptr;
	state.index = 0;

	Output::setQuietNonBuild(m_quiet);

	state.errorCode = CommandPoolErrorCode::None;
}
}
