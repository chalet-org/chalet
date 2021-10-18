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
std::mutex s_mutex;
std::atomic<uint> s_compileIndex = 0;
std::atomic<CommandPoolErrorCode> s_errorCode = CommandPoolErrorCode::None;
std::function<bool()> s_shutdownHandler;

/*****************************************************************************/
bool printCommand(std::string prefix, std::string text, uint total = 0)
{
	std::lock_guard<std::mutex> lock(s_mutex);
	if (total > 0)
	{
		// auto indexStr = std::to_string(s_compileIndex);
		uint index = s_compileIndex;
		// const auto totalStr = std::to_string(total);
		// while (indexStr.size() < totalStr.size())
		// {
		// 	indexStr = " " + indexStr;
		// }

		std::cout << fmt::format("{}  [{}/{}] {}", prefix, index, total, text) << std::endl;
	}
	else
	{
		std::cout << fmt::format("{}  {}", prefix, text) << std::endl;
	}

	s_compileIndex++;

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

	auto onOutput = [&srcFile](std::string inData) {
		if (String::startsWith(srcFile, inData))
			return;

		std::cout << inData << std::flush;
	};

	ProcessOptions options;
	options.stdoutOption = PipeOption::Pipe;
	options.stderrOption = PipeOption::Pipe;
	options.onStdOut = onOutput;
	options.onStdErr = onOutput;

	if (Process::run(command, options) != EXIT_SUCCESS)
	{
		s_errorCode = CommandPoolErrorCode::BuildFailure;
		return false;
	}

	return true;
}

/*****************************************************************************/
bool executeCommandCarriageReturn(StringList command, std::string sourceFile)
{
	UNUSED(sourceFile);

	ProcessOptions options;
	static auto onStdOut = [](std::string inData) {
		String::replaceAll(inData, "\n", "\r\n");
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
		s_errorCode = CommandPoolErrorCode::BuildFailure;
		String::replaceAll(errorOutput, "\n", "\r\n");
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
		s_errorCode = CommandPoolErrorCode::BuildFailure;
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
	if (s_shutdownHandler())
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
bool CommandPool::run(const Target& inTarget, const Settings& inSettings) const
{
	auto& list = inTarget.list;
	auto& pre = inTarget.pre;
	auto& post = inTarget.post;

	auto&& [quiet, showCommmands, msvcCommand, renameAfterCommand] = inSettings;

	::signal(SIGINT, signalHandler);
	::signal(SIGTERM, signalHandler);
	::signal(SIGABRT, signalHandler);

	m_exceptionThrown.clear();
	s_errorCode = CommandPoolErrorCode::None;
	m_quiet = quiet;

	s_shutdownHandler = [this]() -> bool {
		if (s_errorCode != CommandPoolErrorCode::None)
			return false;

		this->m_threadPool.stop();
		s_errorCode = CommandPoolErrorCode::Aborted;
		return true;
	};

	Output::setQuietNonBuild(false);
	// Output::setShowCommandOverride(false);

	auto& executeCommandFunc = msvcCommand ?
		  executeCommandMsvc :
		Environment::isMicrosoftTerminalOrWindowsBash() ?
		  executeCommandCarriageReturn :
		  executeCommand;

	s_compileIndex = 1;
	uint totalCompiles = static_cast<uint>(list.size());
	totalCompiles += static_cast<uint>(pre.size());

	if (!post.command.empty())
		++totalCompiles;

	auto reset = Output::getAnsiStyle(Color::Reset);

	// At the moment, this is only greater than 1 when compiling multiple PCHes for MacOS universal binaries
	for (auto& it : pre)
	{
		if (!it.command.empty())
		{
			auto color = Output::getAnsiStyle(it.color);

			if (!printCommand(
					color + it.symbol + reset,
					color + (showCommmands ? String::join(it.command) : it.output) + reset,
					totalCompiles))
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
			auto color = Output::getAnsiStyle(it.color);

			threadResults.emplace_back(m_threadPool.enqueue(
				printCommand,
				color + it.symbol + reset,
				color + (showCommmands ? String::join(it.command) : it.output) + reset,
				totalCompiles));

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
			if (s_errorCode != CommandPoolErrorCode::None && m_exceptionThrown.empty())
			{
				signalHandler(SIGTERM);
				if (String::equals("build error", err.what()))
				{}
				else
				{
					m_exceptionThrown = fmt::format("exception '{}'", err.what());
					s_errorCode = CommandPoolErrorCode::BuildException;
				}
			}
		}
	}

	if (s_errorCode != CommandPoolErrorCode::None)
	{
		// m_threadPool.stop();
		threadResults.clear();

		return onError();
	}

	if (!post.command.empty())
	{
		// Output::lineBreak();

		auto color = Output::getAnsiStyle(post.color);

		if (!printCommand(
				color + post.symbol + reset,
				color + post.label + ' ' + (showCommmands ? String::join(post.command) : post.output) + reset,
				totalCompiles))
			return onError();

		if (!executeCommandFunc(post.command, post.output))
			return onError();

		if (s_errorCode != CommandPoolErrorCode::None)
		{
			threadResults.clear();
			return onError();
		}

		Output::lineBreak();
	}

	s_shutdownHandler = nullptr;
	s_compileIndex = 0;

	Output::setQuietNonBuild(quiet);
	// Output::setShowCommandOverride(true);

	return true;
}

/*****************************************************************************/
bool CommandPool::onError() const
{
	if (s_errorCode == CommandPoolErrorCode::Aborted)
	{
		Output::msgCommandPoolError("Aborted by user.");
	}
	else if (s_errorCode == CommandPoolErrorCode::BuildException)
	{
		Output::msgCommandPoolError(m_exceptionThrown);
		std::cout << "Terminated running processes." << std::endl;
	}

	Output::setQuietNonBuild(m_quiet);
	// Diagnostic::error("Build error...");
	Output::lineBreak();
	return false;
}
}
