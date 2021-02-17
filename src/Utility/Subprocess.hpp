/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_SUBPROCESS_HPP
#define CHALET_SUBPROCESS_HPP

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
	Subprocess::PipeFunc onStdout = nullptr;
	Subprocess::PipeFunc onStderr = nullptr;
};

}

#endif // CHALET_SUBPROCESS_HPP
