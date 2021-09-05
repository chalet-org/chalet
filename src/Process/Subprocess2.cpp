/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Process/Subprocess2.hpp"

#include <array>
#include <atomic>

#if defined(CHALET_WIN32)
#else
	#include <fcntl.h>
	#include <sys/wait.h>
	#include <unistd.h>
#endif

#include "Libraries/Format.hpp"
#include "Process/ProcessPipe.hpp"
#include "Process/RunningProcess.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/OSTerminal.hpp"
#include "Terminal/Output.hpp"

extern char** environ;

namespace chalet
{
namespace
{
std::vector<RunningProcess*> s_procesess;
std::atomic<int> s_lastErrorCode = 0;
bool s_initialized = false;

/*****************************************************************************/
void removeProcess(const RunningProcess& inProcess)
{
	auto it = s_procesess.end();
	while (it != s_procesess.begin())
	{
		--it;
		RunningProcess* process = (*it);
		if (process->m_pid == inProcess.m_pid)
		{
			it = s_procesess.erase(it);
			return;
		}
	}

	if (s_procesess.empty())
		OSTerminal::reset();
}

/*****************************************************************************/
void subProcessSignalHandler(int inSignal)
{
	auto it = s_procesess.end();
	while (it != s_procesess.begin())
	{
		--it;
		RunningProcess* process = (*it);

		if (static_cast<SigNum>(inSignal) == SigNum::Terminate)
			process->terminate();
		else
			process->sendSignal(static_cast<SigNum>(inSignal));

		it = s_procesess.erase(it);
	}

	OSTerminal::reset();
}
}

/*****************************************************************************/
int Subprocess2::run(const StringList& inCmd, ProcessOptions&& inOptions, const std::uint8_t inBufferSize)
{
	CHALET_TRY
	{
		if (!s_initialized)
		{
			::signal(SIGINT, subProcessSignalHandler);
			::signal(SIGTERM, subProcessSignalHandler);
			::signal(SIGABRT, subProcessSignalHandler);
			s_initialized = true;
		}

		if (inCmd.empty())
		{
			CHALET_THROW(std::runtime_error("Subprocess: Command cannot be empty."));
		}

		if (!Commands::pathExists(inCmd.front()))
		{
			CHALET_THROW(std::runtime_error(fmt::format("Subprocess: Executable not found: {}", inCmd.front())));
		}

		if (inOptions.stdoutOption == PipeOption::StdOut && inOptions.stderrOption == PipeOption::StdErr)
		{
			return RunningProcess::createWithoutPipes(inCmd, inOptions.cwd);
		}
		else
		{
			RunningProcess process;
			if (!process.create(inCmd, inOptions))
				return 1;

			s_procesess.push_back(&process);

			static std::array<char, 256> buffer{ 0 };

			if (inOptions.stdoutOption == PipeOption::Pipe)
				process.read(FileNo::StdOut, buffer, inBufferSize, inOptions.onStdOut);

			if (inOptions.stderrOption == PipeOption::Pipe)
				process.read(FileNo::StdErr, buffer, inBufferSize, inOptions.onStdErr);

			int result = process.waitForResult();

			removeProcess(process);
			s_lastErrorCode = result;

			return result;
		}
	}
	CHALET_CATCH(const std::exception& err)
	{
		CHALET_EXCEPT_ERROR("subprocess2 error: {}", err.what());
		return -1;
	}
}

/*****************************************************************************/
int Subprocess2::getLastExitCode()
{
	return s_lastErrorCode;
}

/*****************************************************************************/
void Subprocess2::haltAllProcesses(const SigNum inSignal)
{
	subProcessSignalHandler(static_cast<std::underlying_type_t<SigNum>>(inSignal));
}
}
