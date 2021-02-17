/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_SUBPROCESS_HPP
#define CHALET_SUBPROCESS_HPP

namespace chalet
{
namespace Subprocess
{
using PipeFunc = std::function<void(const std::string&)>;
int run(const StringList& inCmd, const PipeFunc& onStdout, const PipeFunc& onStderr = nullptr);
}
}

#endif // CHALET_SUBPROCESS_HPP
