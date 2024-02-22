/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include "Process/ProcessOptions.hpp"
#include "Process/SigNum.hpp"

namespace chalet
{
class SubProcess;

struct SubProcessController
{
	static i32 run(const StringList& inCmd, const ProcessOptions& inOptions);
	static bool create(SubProcess& process, const StringList& inCmd, const ProcessOptions& inOptions);
	static i32 getLastExitCodeFromProcess(SubProcess& process);
	static i32 getLastExitCodeFromProcess(SubProcess& process, const bool waitForResult);
	static i32 pollProcessState(SubProcess& process);
	static i32 getLastExitCode();
	static std::string getSystemMessage(const i32 inExitCode);
	static std::string getSignalRaisedMessage(const i32 inExitCode);
	static std::string getSignalNameFromCode(const i32 inExitCode);

private:
	static bool commandIsValid(const StringList& inCmd);
};
}
