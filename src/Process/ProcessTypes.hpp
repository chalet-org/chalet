/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#if defined(CHALET_WIN32)
	#include "Libraries/WindowsApi.hpp"
#else
	#include <unistd.h>
#endif

namespace chalet
{
namespace FileNo
{
#if defined(CHALET_WIN32)
constexpr DWORD StdIn = STD_INPUT_HANDLE;
constexpr DWORD StdOut = STD_OUTPUT_HANDLE;
constexpr DWORD StdErr = STD_ERROR_HANDLE;
#else
constexpr i32 StdIn = STDIN_FILENO;
constexpr i32 StdOut = STDOUT_FILENO;
constexpr i32 StdErr = STDERR_FILENO;
#endif
}

#if defined(CHALET_WIN32)
typedef HANDLE PipeHandle;
typedef DWORD ProcessID;

const PipeHandle kInvalidPipe = INVALID_HANDLE_VALUE;
#else
typedef i32 PipeHandle;
typedef ::pid_t ProcessID;

constexpr PipeHandle kInvalidPipe = -1;
#endif
}
