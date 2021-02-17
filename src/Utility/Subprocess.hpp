/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_SUBPROCESS_HPP
#define CHALET_SUBPROCESS_HPP

#include "Libraries/SubprocessApi.hpp"

namespace chalet
{
struct SubprocessOptions;

namespace Subprocess
{
using PipeFunc = std::function<void(const std::string&)>;
int run(const StringList& inCmd, const SubprocessOptions& inOptions);
}

struct SubprocessOptions
{
	std::string cwd;
	sp::PipeOption stdoutOption = sp::PipeOption::close;
	sp::PipeOption stderrOption = sp::PipeOption::close;
	Subprocess::PipeFunc onStdout = nullptr;
	Subprocess::PipeFunc onStderr = nullptr;
};

}

#endif // CHALET_SUBPROCESS_HPP
