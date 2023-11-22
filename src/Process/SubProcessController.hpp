/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include "Process/ProcessOptions.hpp"
#include "Process/SigNum.hpp"

namespace chalet
{
namespace SubProcessController
{
i32 run(const StringList& inCmd, const ProcessOptions& inOptions, const u8 inBufferSize = 0);
i32 getLastExitCode();
std::string getSystemMessage(const i32 inExitCode);
std::string getSignalRaisedMessage(const i32 inExitCode);
std::string getSignalNameFromCode(const i32 inExitCode);
}
}
