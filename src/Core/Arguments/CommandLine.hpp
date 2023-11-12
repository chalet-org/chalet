/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

namespace chalet
{
struct CommandLineInputs;

namespace CommandLine
{
Unique<CommandLineInputs> read(const i32 argc, const char* argv[], bool& outResult);
}
}
