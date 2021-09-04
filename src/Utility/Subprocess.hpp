/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_SUBPROCESS_HPP
#define CHALET_SUBPROCESS_HPP

#include <signal.h>

#include "Utility/SubprocessOptions.hpp"

namespace chalet
{
namespace Subprocess
{
int getLastExitCode();

int run(const StringList& inCmd, SubprocessOptions&& inOptions);
void haltAllProcesses(const int inSignal = SIGTERM);
}
}

#endif // CHALET_SUBPROCESS_HPP
