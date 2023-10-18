/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_PROCESS_CONTROLLER_HPP
#define CHALET_PROCESS_CONTROLLER_HPP

#include "Process/ProcessOptions.hpp"
#include "Process/SigNum.hpp"

namespace chalet
{
namespace ProcessController
{
int run(const StringList& inCmd, const ProcessOptions& inOptions, const std::uint8_t inBufferSize = 0);
int getLastExitCode();
std::string getSystemMessage(const int inExitCode);
std::string getSignalRaisedMessage(const int inExitCode);
std::string getSignalNameFromCode(const int inExitCode);
void haltAll(const SigNum inSignal = SigNum::Terminate);
}
}

#endif // CHALET_PROCESS_CONTROLLER_HPP
