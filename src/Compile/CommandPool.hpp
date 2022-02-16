/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_COMMAND_POOL_HPP
#define CHALET_COMMAND_POOL_HPP

#include "Libraries/ThreadPool.hpp"
#include "Terminal/Color.hpp"

namespace chalet
{
struct CommandPool
{
	struct Cmd
	{
		std::string output;
		StringList command;
#if defined(CHALET_WIN32)
		std::string outputReplace;
#endif
	};
	using CmdList = std::vector<Cmd>;

	struct Job
	{
		CmdList list;
		uint threads = 0;
	};
	using JobList = std::vector<Unique<CommandPool::Job>>;

	struct Settings
	{
		Color color = Color::Red;
		uint startIndex = 0;
		uint total = 0;
		bool quiet = false;
		bool showCommands = false;
		bool keepGoing = false;
		bool msvcCommand = false;
	};

	explicit CommandPool(const std::size_t inThreads);

	bool runAll(JobList& inJobs, Settings& inSettings);
	bool run(const Job& inTarget, const Settings& inSettings);

private:
	std::string getPrintedText(std::string inText, uint inTotal);
	bool onError();
	void cleanup();

	ThreadPool m_threadPool;

	std::string m_reset;
	std::string m_exceptionThrown;
	bool m_quiet = false;
};
}

#endif // CHALET_COMMAND_POOL_HPP
