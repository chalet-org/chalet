/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_PROCESS_HPP
#define CHALET_PROCESS_HPP

#include "Process/ProcessOptions.hpp"
#include "Process/SigNum.hpp"

namespace chalet
{
namespace Process
{
int run(const StringList& inCmd, const ProcessOptions& inOptions, const std::uint8_t inBufferSize = 0);
int getLastExitCode();
std::string getSystemMessage(const int inExitCode);
void haltAll(const SigNum inSignal = SigNum::Terminate);
}
}

#endif // CHALET_PROCESS_HPP
