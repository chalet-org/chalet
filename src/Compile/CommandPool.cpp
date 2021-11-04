/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/CommandPool.hpp"

#include <csignal>

#include "Process/Process.hpp"
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
	uint index = 0;
	std::atomic<CommandPoolErrorCode> errorCode = CommandPoolErrorCode::None;
	std::function<bool()> shutdownHandler;
} state;

/*****************************************************************************/
bool printCommand(std::string text)
{
	std::lock_guard<std::mutex> lock(s_mutex);

	String::replaceAll(text, '#', std::to_string(state.index));
	std::cout << text << std::endl;

	++state.index;

	return true;
}

/*****************************************************************************/
bool executeCommandMsvc(StringList command, std::string sourceFile)
{
	std::string srcFile;
	{
		auto start = sourceFile.find_last_of('/') + 1;
		auto end = sourceFile.find_last_of('.');
		srcFile = sourceFile.substr(start, end - start);
	}

	std::string output;
	auto onOutput = [&srcFile, &output](std::string inData) {
		if (String::startsWith(srcFile, inData))
			return;

		output += std::move(inData);
	};

	ProcessOptions options;
	options.stdoutOption = PipeOption::Pipe;
	options.stderrOption = PipeOption::Pipe;
	options.onStdOut = onOutput;
	options.onStdErr = onOutput;

	bool result = true;
	if (Process::run(command, options) != EXIT_SUCCESS)
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

/*****************************************************************************/
bool executeCommandCarriageReturn(StringList command, std::string sourceFile)
{
	UNUSED(sourceFile);

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
	if (Process::run(command, options) != EXIT_SUCCESS)
		result = false;

	if (!errorOutput.empty())
	{
		std::lock_guard<std::mutex> lock(s_mutex);
		String::replaceAll(errorOutput, '\n', "\r\n");

		if (!result)
			state.errorCode = CommandPoolErrorCode::BuildFailure;

		auto error = Output::getAnsiStyle(Output::theme().error);
		auto reset = Output::getAnsiStyle(Color::Reset);
		auto cmdString = String::join(command);

		std::cout << fmt::format("{}FAILED: {}{}\r\n", error, reset, cmdString) << errorOutput << std::flush;
	}

	return result;
}

/*****************************************************************************/
bool executeCommand(StringList command, std::string sourceFile)
{
	UNUSED(sourceFile);

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
	if (Process::run(command, options) != EXIT_SUCCESS)
		result = false;

	if (!errorOutput.empty())
	{
		std::lock_guard<std::mutex> lock(s_mutex);
		if (!result)
			state.errorCode = CommandPoolErrorCode::BuildFailure;

		auto error = Output::getAnsiStyle(Output::theme().error);
		auto reset = Output::getAnsiStyle(Color::Reset);
		auto cmdString = String::join(command);

		std::cout << fmt::format("{}FAILED: {}{}\n", error, reset, cmdString) << errorOutput << std::flush;
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
			Process::haltAll(SigNum::Terminate);
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
bool CommandPool::run(const Target& inTarget, const Settings& inSettings)
{
	auto& list = inTarget.list;
	auto& pre = inTarget.pre;
	auto& post = inTarget.post;

	auto&& [cmdColor, quiet, showCommmands, msvcCommand, renameAfterCommand] = inSettings;

	::signal(SIGINT, signalHandler);
	::signal(SIGTERM, signalHandler);
	::signal(SIGABRT, signalHandler);

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

	auto& executeCommandFunc = msvcCommand ?
		  executeCommandMsvc :
		Environment::isMicrosoftTerminalOrWindowsBash() ?
		  executeCommandCarriageReturn :
		  executeCommand;

	state.index = 1;
	uint totalCompiles = static_cast<uint>(list.size());
	totalCompiles += static_cast<uint>(pre.size());

	if (!post.command.empty())
		++totalCompiles;

	m_reset = Output::getAnsiStyle(Color::Reset);
	auto color = Output::getAnsiStyle(cmdColor);

	// At the moment, this is only greater than 1 when compiling multiple PCHes for MacOS universal binaries
	for (auto& it : pre)
	{
		if (!it.command.empty())
		{
			if (!printCommand(getPrintedText(fmt::format("{}{}", color, (showCommmands ? String::join(it.command) : it.output)),
					totalCompiles)))
				return onError();

			if (!executeCommandFunc(it.command, it.output))
				return onError();
		}
	}

	std::vector<std::future<bool>> threadResults;
	for (auto& it : list)
	{
		if (!it.command.empty())
		{
			threadResults.emplace_back(m_threadPool.enqueue(
				printCommand,
				getPrintedText(fmt::format("{}{}", color, (showCommmands ? String::join(it.command) : it.output)),
					totalCompiles)));

			threadResults.emplace_back(m_threadPool.enqueue(executeCommandFunc, it.command, it.output));
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

	if (!post.command.empty())
	{
		// Output::lineBreak();

		// auto postColor = Output::getAnsiStyleForceFormatting(cmdColor, Formatting::Bold);

		if (!printCommand(getPrintedText(fmt::format("{}{} {}", color, post.label, (showCommmands ? String::join(post.command) : post.output)),
				totalCompiles)))
			return onError();

		if (!executeCommandFunc(post.command, post.output))
			return onError();

		if (state.errorCode != CommandPoolErrorCode::None)
		{
			threadResults.clear();
			return onError();
		}

		Output::lineBreak();
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
