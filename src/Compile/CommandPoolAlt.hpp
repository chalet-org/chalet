/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include "System/DefinesExperimentalFeatures.hpp"

#if CHALET_ALT_COMMAND_POOL
	#include "Process/SubProcess.hpp"
	#include "Terminal/Color.hpp"

namespace chalet
{
struct CommandPoolAlt
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
	using JobList = std::vector<Unique<CommandPoolAlt::Job>>;

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

	explicit CommandPoolAlt(const size_t inMaxJobs);
	CHALET_DISALLOW_COPY_MOVE(CommandPoolAlt);
	~CommandPoolAlt();

	bool runAll(JobList& inJobs, Settings& inSettings);
	bool run(const Job& inTarget, const Settings& inSettings);

	const StringList& failures() const;

private:
	SubProcess::OutputBuffer m_buffer;

	struct RunningProcess
	{
		std::string output;
		const StringList* command = nullptr;
	#if defined(CHALET_WIN32)
		const std::string* reference = nullptr;
		const std::string* dependencyFile = nullptr;
	#endif
		SubProcess process;
		ProcessOptions options;

		size_t index = 0;
		i32 exitCode = -1;

		bool result = false;
		bool filterMsvc = false;

		bool createRunningProcess();
		bool pollState(SubProcess::OutputBuffer& buffer);
		void getResultAndPrintOutput(SubProcess::OutputBuffer& buffer);

	private:
		i32 getLastExitCode();
		void printOutput();
	#if defined(CHALET_WIN32)
		void printMsvcOutput();
	#endif
		void updateHandle(SubProcess::OutputBuffer& buffer, const SubProcess::HandleInput& inFileNo, const ProcessOptions::PipeFunc& onRead);
	};

	void printCommand(std::string text);
	std::string getPrintedText(std::string inText, u32 inTotal);
	bool onError();
	void cleanup();

	u32 m_index = 0;

	std::vector<Unique<RunningProcess>> m_processes;

	StringList m_failures;

	std::string m_reset;
	std::string m_exceptionThrown;

	size_t m_maxJobs = 0;

	bool m_quiet = false;
};
}
#endif
