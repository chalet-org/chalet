/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Process/Process.hpp"

#include <array>
#include <atomic>

#include "Libraries/Format.hpp"
#include "Process/ProcessPipe.hpp"
#include "Process/RunningProcess.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/OSTerminal.hpp"
#include "Terminal/Output.hpp"

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
		if (*process == inProcess)
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

		bool success = process->sendSignal(static_cast<SigNum>(inSignal));
		if (success)
			it = s_procesess.erase(it);
	}

	OSTerminal::reset();
}
}

/*****************************************************************************/
int Process::run(const StringList& inCmd, const ProcessOptions& inOptions, const std::uint8_t inBufferSize)
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
			Diagnostic::error("Subprocess: Command cannot be empty.");
			return -1;
		}

		if (!Commands::pathExists(inCmd.front()))
		{
			Diagnostic::error("Subprocess: Executable not found: {}", inCmd.front());
			return -1;
		}

		RunningProcess process;
		if (!process.create(inCmd, inOptions))
			return 1;

		s_procesess.push_back(&process);

		static std::array<char, 256> buffer{ 0 };

		if (inOptions.stdoutOption == PipeOption::Pipe && inOptions.onStdOut != nullptr)
			process.read(FileNo::StdOut, buffer, inBufferSize, inOptions.onStdOut);

		if (inOptions.stderrOption == PipeOption::Pipe && inOptions.onStdErr != nullptr)
			process.read(FileNo::StdErr, buffer, inBufferSize, inOptions.onStdErr);

		int result = process.waitForResult();

		removeProcess(process);
		s_lastErrorCode = result;

		return result;
	}
	CHALET_CATCH(const std::exception& err)
	{
		CHALET_EXCEPT_ERROR("Subprocess error: {}", err.what());
		return -1;
	}
}

/*****************************************************************************/
int Process::getLastExitCode()
{
	return s_lastErrorCode;
}

/*****************************************************************************/
void Process::haltAll(const SigNum inSignal)
{
	subProcessSignalHandler(static_cast<std::underlying_type_t<SigNum>>(inSignal));
}
}
