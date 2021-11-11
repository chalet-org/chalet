/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_PROCESS_PIPE_OPTION_HPP
#define CHALET_PROCESS_PIPE_OPTION_HPP

namespace chalet
{
enum class PipeOption : int
{
	Inherit,
	StdOut,
	StdErr,
	Pipe,
	Close
};

}

#endif // CHALET_PROCESS_PIPE_OPTION_HPP
