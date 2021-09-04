/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_SUBPROCESS_OPTIONS_HPP
#define CHALET_SUBPROCESS_OPTIONS_HPP

#include "Utility/SubprocessTypes.hpp"

namespace chalet
{
struct SubprocessOptions
{
	using PipeFunc = std::function<void(std::string_view /* output */)>;
	using CreateFunc = std::function<void(int /* pid */)>;

	std::string cwd;
	PipeOption stdoutOption = PipeOption::Close;
	PipeOption stderrOption = PipeOption::Close;
	PipeFunc onStdOut = nullptr;
	PipeFunc onStdErr = nullptr;
	CreateFunc onCreate = nullptr;
};
}

#endif // CHALET_SUBPROCESS_OPTIONS_HPP
