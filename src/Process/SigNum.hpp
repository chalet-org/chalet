/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_SIG_NUM_HPP
#define CHALET_SIG_NUM_HPP

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
	// SIGIOT = 6,
	// SIGBUS = 7,
	FloatingPointException = SIGFPE,
	Kill = 9,
	// SIGUSR1 = 10,
	SegmentationViolation = SIGSEGV,
	// SIGUSR2 = 12,
	BrokenPipe = 13,
	Alarm = 14,
	Terminate = SIGTERM,
	// SIGSTKFLT = 16,
	// SIGCHLD = 17,
	// SIGCONT = 18,
	// SIGSTOP = 19,
	// SIGTSTP = 20,
	// SIGTTIN = 21,
	// SIGTTOU = 22,
	// SIGURG = 23,
	// SIGXCPU = 24,
	// SIGXFSZ = 25,
	// SIGVTALRM = 26,
	// SIGPROF = 27,
	// SIGWINCH = 28,
	// SIGIO = 29
};
}

#endif // CHALET_SIG_NUM_HPP
