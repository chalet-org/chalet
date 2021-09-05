/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_PROCESS_TYPES_HPP
#define CHALET_PROCESS_TYPES_HPP

#if defined(CHALET_WIN32)
	#include "Libraries/WindowsApi.hpp"
#else
	#include <unistd.h>
#endif

namespace chalet
{
namespace FileNo
{
enum
{
	StdIn = STDIN_FILENO,
	StdOut = STDOUT_FILENO,
	StdErr = STDERR_FILENO
};
}

#if defined(CHALET_WIN32)
typedef HANDLE PipeHandle;
typedef DWORD ProcessID;

constexpr PipeHandle kInvalidPipe = INVALID_HANDLE_VALUE;
#else
typedef int PipeHandle;
typedef ::pid_t ProcessID;

constexpr PipeHandle kInvalidPipe = -1;
#endif
}

#endif // CHALET_PROCESS_TYPES_HPP
