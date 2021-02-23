/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_SUBPROCESS_HPP
#define CHALET_SUBPROCESS_HPP

#include <signal.h>

#include "Utility/SubprocessTypes.hpp"

namespace chalet
{
struct SubprocessOptions;

namespace Subprocess
{
using PipeFunc = std::function<void(std::string /* output */)>;
using CreateFunc = std::function<void(int /* pid */)>;

int run(const StringList& inCmd, SubprocessOptions&& inOptions);
void haltAllProcesses(const int inSignal = SIGTERM);
}

struct SubprocessOptions
{
	std::string cwd;
	PipeOption stdoutOption = PipeOption::Close;
	PipeOption stderrOption = PipeOption::Close;
	Subprocess::PipeFunc onStdOut = nullptr;
	Subprocess::PipeFunc onStdErr = nullptr;
	Subprocess::CreateFunc onCreate = nullptr;
	EnvMap env;
};

}

#endif // CHALET_SUBPROCESS_HPP
