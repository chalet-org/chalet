/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include <signal.h>

namespace chalet
{
enum class SigNum : int
{
	HangUp = 1,
	Interrupt = SIGINT,
	Quit = 3,
	IllegalInstruction = SIGILL,
	Trap = 5,
#if defined(CHALET_WIN32)
	Abort = SIGABRT_COMPAT, // sigh
#else
	Abort = SIGABRT,
#endif
	FloatingPointException = SIGFPE,
	Kill = 9,
	SegmentationViolation = SIGSEGV,
	BrokenPipe = 13,
	Alarm = 14,
	Terminate = SIGTERM,
};
}
