/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Process/SubProcessController.hpp"

#include <array>
#include <mutex>

#include "Process/ProcessPipe.hpp"
#include "Process/SubProcess.hpp"
#include "System/Files.hpp"
#include "System/SignalHandler.hpp"
#include "Terminal/Output.hpp"

#if defined(CHALET_WIN32)
	#include "Terminal/WindowsTerminal.hpp"
#endif

namespace chalet
{
std::mutex s_mutex;
namespace
{
struct
{
	std::vector<SubProcess*> procesess;
	i32 lastErrorCode = 0;
	bool initialized = false;
} state;

/*****************************************************************************/
void addProcess(SubProcess& inProcess)
{
	std::lock_guard<std::mutex> lock(s_mutex);
	state.procesess.push_back(&inProcess);
}

/*****************************************************************************/
void removeProcess(const SubProcess& inProcess)
{
	std::lock_guard<std::mutex> lock(s_mutex);
	if (state.procesess.empty())
	{
		auto it = state.procesess.end();
		while (it != state.procesess.begin())
		{
			--it;
			SubProcess* process = (*it);
			if (*process == inProcess)
			{
				it = state.procesess.erase(it);
				return;
			}
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
	std::lock_guard<std::mutex> lock(s_mutex);
	auto it = state.procesess.end();
	while (it != state.procesess.begin())
	{
		--it;
		SubProcess* process = (*it);

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
i32 SubProcessController::run(const StringList& inCmd, const ProcessOptions& inOptions, const u8 inBufferSize)
{
	CHALET_TRY
	{
		if (!state.initialized)
		{
			std::lock_guard<std::mutex> lock(s_mutex);

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

		if (!Files::pathExists(inCmd.front()))
		{
			Diagnostic::error("Subprocess: Executable not found: {}", inCmd.front());
			return -1;
		}

		SubProcess process;
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
i32 SubProcessController::getLastExitCode()
{
	return state.lastErrorCode;
}

/*****************************************************************************/
std::string SubProcessController::getSystemMessage(const i32 inExitCode)
{
	if (inExitCode == 0)
		return std::string();

	return SubProcess::getErrorMessageFromCode(inExitCode);
}

/*****************************************************************************/
std::string SubProcessController::getSignalRaisedMessage(const i32 inExitCode)
{
	return SubProcess::getErrorMessageFromSignalRaised(inExitCode < 0 ? inExitCode * -1 : inExitCode);
}

/*****************************************************************************/
std::string SubProcessController::getSignalNameFromCode(const i32 inExitCode)
{
	return SubProcess::getSignalNameFromCode(inExitCode);
}

/*****************************************************************************/
void SubProcessController::haltAll(const SigNum inSignal)
{
	subProcessSignalHandler(static_cast<std::underlying_type_t<SigNum>>(inSignal));
}
}