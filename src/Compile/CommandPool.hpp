/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include "Libraries/ThreadPool.hpp"
#include "Terminal/Color.hpp"

namespace chalet
{
struct CommandPool
{
	struct Cmd
	{
		std::string output;
		std::string reference;
#if defined(CHALET_WIN32)
		// msvc only
		std::string dependency;
#endif
		StringList command;
	};
	using CmdList = std::vector<Cmd>;

	struct Job
	{
		CmdList list;
		u32 threads = 0;
	};
	using JobList = std::vector<Unique<CommandPool::Job>>;

	struct Settings
	{
		Color color = Color::Red;
		u32 startIndex = 0;
		u32 total = 0;
		bool quiet = false;
		bool showCommands = false;
		bool keepGoing = false;
		bool msvcCommand = false;
	};

	explicit CommandPool(const size_t inThreads);
	CHALET_DISALLOW_COPY_MOVE(CommandPool);
	~CommandPool();

	bool runAll(JobList& inJobs, Settings& inSettings);
	bool run(const Job& inTarget, const Settings& inSettings);

	const StringList& failures() const;

private:
	std::string getPrintedText(std::string inText, u32 inTotal);
	bool onError();
	void cleanup();

	ThreadPool m_threadPool;

	StringList m_failures;

	std::string m_reset;
	std::string m_exceptionThrown;
	bool m_quiet = false;
};
}
