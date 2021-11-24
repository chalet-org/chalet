/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Process/ProcessController.hpp"

#include <array>
#include <atomic>
#include <mutex>

#include "Libraries/Format.hpp"
#include "Process/Process.hpp"
#include "Process/ProcessPipe.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Output.hpp"

#if defined(CHALET_WIN32)
	#include "Terminal/WindowsTerminal.hpp"
#endif

namespace chalet
{
namespace
{
static std::mutex s_mutex;
static struct
{
	std::vector<Process*> procesess;
	int lastErrorCode = 0;
	bool initialized = false;
} state;

/*****************************************************************************/
void removeProcess(const Process& inProcess)
{
	std::lock_guard<std::mutex> lock(s_mutex);
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
void subProcessSignalHandler(int inSignal)
{
	std::lock_guard<std::mutex> lock(s_mutex);
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
	WindowsTerminal::reset();
#endif
}
}

/*****************************************************************************/
int ProcessController::run(const StringList& inCmd, const ProcessOptions& inOptions, const std::uint8_t inBufferSize)
{
	CHALET_TRY
	{
		if (!state.initialized)
		{
			::signal(SIGINT, subProcessSignalHandler);
			::signal(SIGTERM, subProcessSignalHandler);
			::signal(SIGABRT, subProcessSignalHandler);
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

		state.procesess.push_back(&process);

		static std::array<char, 256> buffer{ 0 };

		{
			// std::lock_guard<std::mutex> lock(s_mutex);
			if (inOptions.stdoutOption == PipeOption::Pipe && inOptions.onStdOut != nullptr)
				process.read(FileNo::StdOut, buffer, inBufferSize, inOptions.onStdOut);

			if (inOptions.stderrOption == PipeOption::Pipe && inOptions.onStdErr != nullptr)
				process.read(FileNo::StdErr, buffer, inBufferSize, inOptions.onStdErr);
		}

		int result = process.waitForResult();

		removeProcess(process);
		state.lastErrorCode = result;

		return result;
	}
	CHALET_CATCH(const std::exception& err)
	{
		CHALET_EXCEPT_ERROR("Subprocess error: {}", err.what());
		return -1;
	}
}

/*****************************************************************************/
int ProcessController::getLastExitCode()
{
	return state.lastErrorCode;
}

/*****************************************************************************/
std::string ProcessController::getSystemMessage(const int inExitCode)
{
	if (inExitCode == 0)
		return std::string();

	return Process::getErrorMessageFromCode(inExitCode);
}

/*****************************************************************************/
void ProcessController::haltAll(const SigNum inSignal)
{
	subProcessSignalHandler(static_cast<std::underlying_type_t<SigNum>>(inSignal));
}
}
