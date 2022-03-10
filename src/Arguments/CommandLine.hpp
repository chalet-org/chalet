/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_COMMAND_LINE_HPP
#define CHALET_COMMAND_LINE_HPP

namespace chalet
{
struct CommandLineInputs;

namespace CommandLine
{
Unique<CommandLineInputs> read(const int argc, const char* argv[], bool& outResult);
}
}

#endif // CHALET_ARGUMENT_READER_HPP
