/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/CommandPool.hpp"

#include <csignal>

#include "Terminal/Commands.hpp"
#include "Terminal/Output.hpp"
#include "Utility/String.hpp"
#include "Utility/Subprocess.hpp"

namespace chalet
{
/*****************************************************************************/
namespace
{
std::mutex s_mutex;
std::atomic<uint> s_compileIndex = 0;
std::function<void()> s_shutdownHandler;

/*****************************************************************************/
bool printCommand(std::string prefix, std::string text, uint total = 0)
{
	std::unique_lock<std::mutex> lock(s_mutex);
	if (total > 0)
	{
		auto indexStr = std::to_string(s_compileIndex);
		const auto totalStr = std::to_string(total);
		while (indexStr.size() < totalStr.size())
		{
			indexStr = " " + indexStr;
		}

		std::cout << fmt::format("{}  [{}/{}] {}", prefix, indexStr, total, text) << std::endl;
	}
	else
	{
		std::cout << fmt::format("{}  {}", prefix, text) << std::endl;
	}

	s_compileIndex++;

	return true;
}

/*****************************************************************************/
bool executeCommandMsvc(StringList command, std::string sourceFile, bool generateDependencies)
{
	std::string srcFile;
	{
		auto start = sourceFile.find_last_of('/') + 1;
		auto end = sourceFile.find_last_of('.');
		srcFile = sourceFile.substr(start, end - start);
	}

	SubprocessOptions options;
	options.stdoutOption = PipeOption::Pipe;
	options.stderrOption = PipeOption::StdErr;
	options.onStdOut = [&srcFile](std::string_view inData) {
		std::string data(inData);
		if (String::startsWith(srcFile, data))
			return;

		std::cout << std::move(data) << std::flush;
	};

	if (Subprocess::run(command, std::move(options)) != EXIT_SUCCESS)
		return false;

	if (!generateDependencies)
		return true;

	return true;
}

/*****************************************************************************/
bool executeCommand(StringList command, std::string sourceFile, bool generateDependencies)
{
	UNUSED(sourceFile);

	SubprocessOptions options;
	options.stdoutOption = PipeOption::StdOut;
	options.stderrOption = PipeOption::StdErr;

	if (Subprocess::run(command, std::move(options)) != EXIT_SUCCESS)
		return false;

	if (!generateDependencies)
		return true;

	return true;
}

/*****************************************************************************/
void signalHandler(int inSignal)
{
	Subprocess::haltAllProcesses(inSignal);
	s_shutdownHandler();
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

	s_shutdownHandler = [this]() {
		this->m_threadPool.stop();
		this->m_canceled = true;
	};

	auto onError = [quiet = quiet]() -> bool {
		Output::setQuietNonBuild(quiet);
		Diagnostic::error("Build error...");
		return false;
	};

	Output::setQuietNonBuild(false);
	// Output::setShowCommandOverride(false);

	auto executeCommandFunc = msvcCommand ? executeCommandMsvc : executeCommand;

	s_compileIndex = 1;
	uint totalCompiles = static_cast<uint>(list.size());

	auto reset = Output::getAnsiReset();

	// At the moment, this is only greater than 1 when compiling multiple PCHes for MacOS universal binaries
	for (auto& it : pre)
	{
		if (!it.command.empty())
		{
			totalCompiles++;

			auto color = Output::getAnsiStyle(it.color);

			if (!printCommand(
					color + it.symbol + reset,
					color + (showCommmands ? String::join(it.command) : it.output) + reset,
					totalCompiles))
				return onError();

			if (!executeCommandFunc(it.command, it.output, renameAfterCommand))
				return onError();
		}
	}

	bool buildFailed = false;
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

			threadResults.emplace_back(m_threadPool.enqueue(executeCommandFunc, it.command, it.output, renameAfterCommand));
		}
	}

	for (auto& tr : threadResults)
	{
		CHALET_TRY
		{
			if (!tr.get())
			{
				signalHandler(SIGTERM);
				buildFailed = true;
				break;
			}
		}
		CHALET_CATCH(std::future_error & err)
		{
			CHALET_EXCEPT_ERROR(err.what());
			return onError();
		}
	}

	if (buildFailed)
	{
		m_threadPool.stop();
		threadResults.clear();
		return onError();
	}

	if (!post.command.empty())
	{
		// Output::lineBreak();

		auto color = Output::getAnsiStyle(post.color);

		if (!printCommand(
				color + post.symbol + reset,
				post.label + ' ' + color + (showCommmands ? String::join(post.command) : post.output) + reset))
			return onError();

		if (!executeCommandFunc(post.command, post.output, renameAfterCommand))
			return onError();

		if (buildFailed)
		{
			threadResults.clear();
			return onError();
		}

		Output::lineBreak();
	}

	Output::setQuietNonBuild(quiet);
	// Output::setShowCommandOverride(true);

	const bool completed = !m_canceled;
	return completed;
}
}
