/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_SUBPROCESS2_HPP
#define CHALET_SUBPROCESS2_HPP

#include "Utility/SubprocessOptions.hpp"

namespace chalet
{
namespace Subprocess2
{
int run(const StringList& inCmd, SubprocessOptions&& inOptions);
int getLastExitCode();
void haltAllProcesses(const int inSignal = SIGTERM);
}
}

#endif // CHALET_SUBPROCESS2_HPP
