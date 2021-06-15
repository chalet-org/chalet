/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/CommandPool.hpp"

#include "Libraries/Format.hpp"
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
bool printCommand(std::string output, StringList command, Color inColor, std::string symbol, bool cleanOutput, uint total = 0)
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

		std::cout << Output::getAnsiStyle(inColor) << fmt::format("{}  ", symbol) << Output::getAnsiReset() << fmt::format("[{}/{}] ", indexStr, total);
	}
	else
	{
		std::cout << Output::getAnsiStyle(inColor) << fmt::format("{}  ", symbol);
	}

	if (cleanOutput)
		Output::print(inColor, output);
	else
		Output::print(inColor, command);

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

		std::cout << inData << std::flush;
	};

	if (Subprocess::run(command, std::move(options)) != EXIT_SUCCESS)
		return false;

	if (!generateDependencies)
		return true;

	if (!renameFrom.empty() && !renameTo.empty())
	{
		std::unique_lock<std::mutex> lock(s_mutex);
		return Commands::rename(renameFrom, renameTo);
	}

	return true;
}

/*****************************************************************************/
bool executeCommand(StringList command, std::string renameFrom, std::string renameTo, bool generateDependencies)
{
	if (!Commands::subprocess(command))
		return false;

	if (!generateDependencies)
		return true;

	if (!renameFrom.empty() && !renameTo.empty())
	{
		std::unique_lock<std::mutex> lock(s_mutex);
		return Commands::rename(renameFrom, renameTo);
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

	auto&& [quiet, cleanOutput, msvcCommand, renameAfterCommand] = inSettings;

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

	auto executeCommandFunc = msvcCommand ? executeCommandMsvc : executeCommand;

	s_compileIndex = 1;
	uint totalCompiles = static_cast<uint>(list.size());

	if (!pre.command.empty())
	{
		totalCompiles++;

		if (!printCommand(pre.output, pre.command, pre.color, pre.symbol, cleanOutput, totalCompiles))
			return onError();

		if (!executeCommandFunc(pre.command, pre.renameFrom, pre.renameTo, renameAfterCommand))
			return onError();
	}

	bool buildFailed = false;
	std::vector<std::future<bool>> threadResults;
	for (auto& it : list)
	{
		threadResults.emplace_back(m_threadPool.enqueue(printCommand, it.output, it.command, it.color, it.symbol, cleanOutput, totalCompiles));
		threadResults.emplace_back(m_threadPool.enqueue(executeCommandFunc, it.command, it.renameFrom, it.renameTo, renameAfterCommand));
	}

	for (auto& tr : threadResults)
	{
		try
		{
			if (!tr.get())
			{
				signalHandler(SIGTERM);
				buildFailed = true;
				break;
			}
		}
		catch (std::future_error& err)
		{
			Diagnostic::error(err.what());
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
		if (!printCommand(post.output, post.command, post.color, post.symbol, cleanOutput))
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

	const bool completed = !m_canceled;
	return completed;
}
}
