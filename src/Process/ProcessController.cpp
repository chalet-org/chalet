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
#include "Terminal/Commands.hpp"
#include "Terminal/Output.hpp"
#include "Utility/SignalHandler.hpp"

#if defined(CHALET_WIN32)
	#include "Terminal/WindowsTerminal.hpp"
#endif

namespace chalet
{
namespace
{
struct
{
	int lastErrorCode = 0;
} state;
}

/*****************************************************************************/
int ProcessController::run(const StringList& inCmd, const ProcessOptions& inOptions, const std::uint8_t inBufferSize)
{
	CHALET_TRY
	{
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

		{
			std::array<char, 128> buffer{ 0 };
			if (inOptions.stdoutOption == PipeOption::Pipe || inOptions.stdoutOption == PipeOption::Close)
				process.read(FileNo::StdOut, buffer, inBufferSize, inOptions.onStdOut);

			if (inOptions.stderrOption == PipeOption::Pipe || inOptions.stderrOption == PipeOption::Close)
				process.read(FileNo::StdErr, buffer, inBufferSize, inOptions.onStdErr);
		}

		state.lastErrorCode = process.waitForResult();

		return state.lastErrorCode;
	}
	CHALET_CATCH(const std::exception& err)
	{
		Diagnostic::error("Subprocess error: {}", err.what());
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
std::string ProcessController::getSignalRaisedMessage(const int inExitCode)
{
	return Process::getErrorMessageFromSignalRaised(inExitCode < 0 ? inExitCode * -1 : inExitCode);
}

/*****************************************************************************/
std::string ProcessController::getSignalNameFromCode(const int inExitCode)
{
	return Process::getSignalNameFromCode(inExitCode);
}
}
