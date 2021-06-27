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
bool executeCommandMsvc(StringList command, std::string renameFrom, std::string renameTo, bool generateDependencies)
{
	std::string srcFile;
	{
		auto start = renameFrom.find_last_of('/') + 1;
		auto end = renameFrom.find_last_of('.');
		srcFile = renameFrom.substr(start, end - start);
	}

	SubprocessOptions options;
	options.stdoutOption = PipeOption::Pipe;
	options.stderrOption = PipeOption::StdErr;
	options.onStdOut = [&srcFile](std::string inData) {
		if (String::startsWith(srcFile, inData))
			return;

		std::cout << std::move(inData) << std::flush;
	};

	if (Subprocess::run(command, std::move(options)) != EXIT_SUCCESS)
		return false;

	if (!generateDependencies)
		return true;

	if (!renameFrom.empty() && !renameTo.empty())
	{
		std::unique_lock<std::mutex> lock(s_mutex);
		CHALET_TRY
		{
			fs::path from(renameFrom);
			if (!fs::exists(from))
				return true;

			fs::path to(renameTo);
			if (fs::exists(to))
				fs::remove(to);

			fs::rename(from, to);
		}
		CHALET_CATCH(const std::exception&)
		{
		}
	}

	return true;
}

/*****************************************************************************/
bool executeCommand(StringList command, std::string renameFrom, std::string renameTo, bool generateDependencies)
{
	SubprocessOptions options;
	options.stdoutOption = PipeOption::StdOut;
	options.stderrOption = PipeOption::StdErr;

	if (Subprocess::run(command, std::move(options)) != EXIT_SUCCESS)
		return false;

	if (!generateDependencies)
		return true;

	if (!renameFrom.empty() && !renameTo.empty())
	{
		std::unique_lock<std::mutex> lock(s_mutex);
		CHALET_TRY
		{
			if (!fs::exists(renameFrom))
				return true;

			if (fs::exists(renameTo))
				fs::remove(renameTo);

			fs::rename(renameFrom, renameTo);
		}
		CHALET_CATCH(const std::exception&)
		{
		}
	}

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
		return false;
	};

	Output::setQuietNonBuild(false);
	// Output::setShowCommandOverride(false);

	auto executeCommandFunc = msvcCommand ? executeCommandMsvc : executeCommand;

	s_compileIndex = 1;
	uint totalCompiles = static_cast<uint>(list.size());

	auto reset = Output::getAnsiReset();

	if (!pre.command.empty())
	{
		totalCompiles++;

		auto color = Output::getAnsiStyle(pre.color);

		if (!printCommand(
				color + pre.symbol + reset,
				color + (showCommmands ? String::join(pre.command) : pre.output) + reset,
				totalCompiles))
			return onError();

		if (!executeCommandFunc(pre.command, pre.renameFrom, pre.renameTo, renameAfterCommand))
			return onError();
	}

	bool buildFailed = false;
	std::vector<std::future<bool>> threadResults;
	for (auto& it : list)
	{
		auto color = Output::getAnsiStyle(it.color);

		threadResults.emplace_back(m_threadPool.enqueue(
			printCommand,
			color + it.symbol + reset,
			color + (showCommmands ? String::join(it.command) : it.output) + reset,
			totalCompiles));

		threadResults.emplace_back(m_threadPool.enqueue(executeCommandFunc, it.command, it.renameFrom, it.renameTo, renameAfterCommand));
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
		threadResults.clear();
		return onError();
	}

	Output::lineBreak();

	if (!post.command.empty())
	{
		auto color = Output::getAnsiStyle(post.color);

		if (!printCommand(
				color + post.symbol + reset,
				color + (showCommmands ? String::join(post.command) : post.output) + reset))
			return onError();

		if (!executeCommandFunc(post.command, post.renameFrom, post.renameTo, renameAfterCommand))
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
