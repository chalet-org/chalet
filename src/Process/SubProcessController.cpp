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
namespace
{
struct
{
	i32 lastErrorCode = 0;
} state;
}

/*****************************************************************************/
i32 SubProcessController::run(const StringList& inCmd, const ProcessOptions& inOptions)
{
	CHALET_TRY
	{
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

		if (inOptions.waitForResult)
		{
			if (inOptions.stdoutOption == PipeOption::Pipe || inOptions.stdoutOption == PipeOption::Close)
				process.read(FileNo::StdOut, inOptions.onStdOut);

			if (inOptions.stderrOption == PipeOption::Pipe || inOptions.stderrOption == PipeOption::Close)
				process.read(FileNo::StdErr, inOptions.onStdErr);
		}

		state.lastErrorCode = inOptions.waitForResult ? process.waitForResult() : 0;

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
}
