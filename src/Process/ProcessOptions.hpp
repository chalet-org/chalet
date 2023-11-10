/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include "Process/PipeOption.hpp"

namespace chalet
{
struct ProcessOptions
{
	using PipeFunc = std::function<void(std::string /* output */)>;
	using CreateFunc = std::function<void(i32 /* pid */)>;

	std::string cwd;
	PipeFunc onStdOut = nullptr;
	PipeFunc onStdErr = nullptr;
	CreateFunc onCreate = nullptr;

	PipeOption stdinOption = PipeOption::Close;
	PipeOption stdoutOption = PipeOption::Close;
	PipeOption stderrOption = PipeOption::Close;
};
}
