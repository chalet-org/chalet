/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Process/ProcessController.hpp"

#include <array>
#include <atomic>
#include <mutex>

#include "Process/Process.hpp"
#include "Process/ProcessPipe.hpp"
#include "System/SignalHandler.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Output.hpp"

#if defined(CHALET_WIN32)
	#include "Terminal/WindowsTerminal.hpp"
#endif

namespace chalet
{
namespace
{
struct
{
	std::vector<Process*> procesess;
	i32 lastErrorCode = 0;
	bool initialized = false;
} state;

/*****************************************************************************/
void addProcess(Process& inProcess)
{
	state.procesess.push_back(&inProcess);
}

/*****************************************************************************/
void removeProcess(const Process& inProcess)
{
	auto it = state.procesess.end();
	while (it != state.procesess.begin())
	{
		--it;
		Process* process = (*it);
		if (*process == inProcess)
		{
			it = state.procesess.erase(it);
			return;
		}
	}

#if defined(CHALET_WIN32)
	if (state.procesess.empty())
		WindowsTerminal::reset();
#endif
}

/*****************************************************************************/
void subProcessSignalHandler(i32 inSignal)
{
	auto it = state.procesess.end();
	while (it != state.procesess.begin())
	{
		--it;
		Process* process = (*it);

		bool success = process->sendSignal(static_cast<SigNum>(inSignal));
		if (success)
			it = state.procesess.erase(it);
	}

#if defined(CHALET_WIN32)
	if (state.procesess.empty())
		WindowsTerminal::reset();
#endif
}
}

/*****************************************************************************/
i32 ProcessController::run(const StringList& inCmd, const ProcessOptions& inOptions, const u8 inBufferSize)
{
	CHALET_TRY
	{
		if (!state.initialized)
		{
			SignalHandler::add(SIGINT, subProcessSignalHandler);
			SignalHandler::add(SIGTERM, subProcessSignalHandler);
			SignalHandler::add(SIGABRT, subProcessSignalHandler);

			state.initialized = true;
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

		Process process;
		if (!process.create(inCmd, inOptions))
		{
			state.lastErrorCode = process.waitForResult();
			return state.lastErrorCode;
		}

		addProcess(process);

		{
			std::array<char, 128> buffer{ 0 };
			if (inOptions.stdoutOption == PipeOption::Pipe || inOptions.stdoutOption == PipeOption::Close)
				process.read(FileNo::StdOut, buffer, inBufferSize, inOptions.onStdOut);

			if (inOptions.stderrOption == PipeOption::Pipe || inOptions.stderrOption == PipeOption::Close)
				process.read(FileNo::StdErr, buffer, inBufferSize, inOptions.onStdErr);
		}

		state.lastErrorCode = process.waitForResult();

		removeProcess(process);

		return state.lastErrorCode;
	}
	CHALET_CATCH(const std::exception& err)
	{
		Diagnostic::error("Subprocess error: {}", err.what());
		return -1;
	}
}

/*****************************************************************************/
i32 ProcessController::getLastExitCode()
{
	return state.lastErrorCode;
}

/*****************************************************************************/
std::string ProcessController::getSystemMessage(const i32 inExitCode)
{
	if (inExitCode == 0)
		return std::string();

	return Process::getErrorMessageFromCode(inExitCode);
}

/*****************************************************************************/
std::string ProcessController::getSignalRaisedMessage(const i32 inExitCode)
{
	return Process::getErrorMessageFromSignalRaised(inExitCode < 0 ? inExitCode * -1 : inExitCode);
}

/*****************************************************************************/
std::string ProcessController::getSignalNameFromCode(const i32 inExitCode)
{
	return Process::getSignalNameFromCode(inExitCode);
}

/*****************************************************************************/
void ProcessController::haltAll(const SigNum inSignal)
{
	subProcessSignalHandler(static_cast<std::underlying_type_t<SigNum>>(inSignal));
}
}
