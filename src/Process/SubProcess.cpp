/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Process/SubProcess.hpp"

#if defined(CHALET_WIN32)
#else
	#include <sys/wait.h>
	#include <unistd.h>
	#include <string.h>
	#include <array>
	#include <signal.h>
#endif

#include "Utility/String.hpp"

#if !defined(CHALET_MSVC)
extern char** environ;
#endif

namespace chalet
{
#if defined(CHALET_WIN32)
namespace
{
/*****************************************************************************/
std::string escapeShellArgument(const std::string& inArg)
{
	bool needsQuote = inArg.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890._-+/") != std::string::npos;
	if (!needsQuote)
		return inArg;

	std::string result = "\"";
	for (auto& c : inArg)
	{
		if (c == '\"' || c == '\\')
			result += '\\';

		result += c;
	}
	result += "\"";

	return result;
}

/*****************************************************************************/
std::string getWindowsArguments(const StringList& inCmd)
{
	std::string args;
	for (size_t i = 0; i < inCmd.size(); ++i)
	{
		if (i > 0)
			args += ' ';

		args += escapeShellArgument(inCmd[i]);
	}

	return args;
}
}

/*****************************************************************************/
i32 SubProcess::waitForResult()
{
	if (m_pid == 0)
	{
		DWORD error = ::GetLastError();
		return static_cast<i32>(error);
	}

	DWORD waitMs = INFINITE;
	DWORD result = ::WaitForSingleObject(m_processInfo.hProcess, waitMs);
	if (result == WAIT_TIMEOUT)
	{
		DWORD error = ::GetLastError();
		Diagnostic::error("WaitForSingleObject WAIT_TIMEOUT error: {}", error);
		return -1;
	}
	else if (result == WAIT_ABANDONED)
	{
		DWORD error = ::GetLastError();
		Diagnostic::error("WaitForSingleObject WAIT_ABANDONED error: {}", error);
		return -1;
	}
	else if (result == WAIT_FAILED)
	{
		DWORD error = ::GetLastError();
		Diagnostic::error("WaitForSingleObject WAIT_FAILED error: {}", error);
		return -1;
	}

	if (result != WAIT_OBJECT_0)
	{
		DWORD error = ::GetLastError();
		Diagnostic::error("WaitForSingleObject error: {}", error);
		return -1;
	}

	DWORD exitCode;
	bool ret = ::GetExitCodeProcess(m_processInfo.hProcess, &exitCode) == TRUE;
	if (!ret)
	{
		DWORD error = ::GetLastError();
		Diagnostic::error("GetExitCodeProcess error: {}", error);
		return -1;
	}

	close();
	return static_cast<i32>(exitCode);
}

/*****************************************************************************/
std::string SubProcess::getErrorMessageFromCode(const i32 inCode)
{
	DWORD messageId = static_cast<DWORD>(inCode);
	if (messageId == 0)
		return std::string();

	LPSTR messageBuffer = NULL;
	DWORD dwFlags = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS;
	DWORD size = FormatMessageA(dwFlags,
		NULL,
		messageId,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPSTR)&messageBuffer,
		0,
		NULL);

	std::string message(messageBuffer, static_cast<size_t>(size));

	LocalFree(messageBuffer);

	return message;
}
#else
/*****************************************************************************/
i32 SubProcess::waitForResult()
{
	i32 exitCode;
	while (true)
	{
		ProcessID child = ::waitpid(m_pid, &exitCode, 0);
		if (child == -1 && errno == EINTR)
			continue;

		break;
	}

	i32 result = getReturnCode(exitCode);

	close();
	return result;
}

/*****************************************************************************/
i32 SubProcess::getReturnCode(const i32 inExitCode)
{
	// Note, we return signals as a negative value to differentiate them later
	if (WIFEXITED(inExitCode))
		return WEXITSTATUS(inExitCode); // system messages
	else if (WIFSIGNALED(inExitCode))
		return -WTERMSIG(inExitCode); // signals
	else
		return 1;
}

/*****************************************************************************/
std::string SubProcess::getErrorMessageFromCode(const i32 inCode)
{
	#if defined(CHALET_MACOS)
	std::array<char, 256> buffer;
	buffer.fill(0);
	UNUSED(::strerror_r(inCode, buffer.data(), buffer.size()));
	return std::string(buffer.data());
	#else
	char* errResult = ::strerror(inCode);
	return std::string(errResult);
	#endif
}

/*****************************************************************************/
SubProcess::CmdPtrArray SubProcess::getCmdVector(const StringList& inCmd)
{
	CmdPtrArray cmd;

	cmd.reserve(inCmd.size() + 1);
	for (size_t i = 0; i < inCmd.size(); ++i)
	{
		cmd.emplace_back(const_cast<char*>(inCmd.at(i).data()));
	}
	cmd.push_back(nullptr);

	return cmd;
}

#endif

/*****************************************************************************/
std::string SubProcess::getErrorMessageFromSignalRaised(const i32 inCode)
{
	switch (inCode)
	{
#if defined(SIGHUP)
		case SIGHUP: return "Hangup";
#endif
#if defined(SIGINT)
		case SIGINT: return "Interrupt";
#endif
#if defined(SIGQUIT)
		case SIGQUIT: return "Quit";
#endif
#if defined(SIGILL)
		case SIGILL: return "Illegal hardware instruction";
#endif
#if defined(SIGTRAP)
		case SIGTRAP: return "Trace trap";
#endif
#if defined(SIGABRT)
		case SIGABRT: return "Abort";
#endif
#if defined(SIGPOLL)
		case SIGPOLL: return "Pollable event occurred";
#endif
#if defined(SIGEMT)
		case SIGEMT: return "EMT instruction";
#endif
#if defined(SIGFPE)
		case SIGFPE: return "Floating point exception";
#endif
#if defined(SIGKILL)
		case SIGKILL: return "Killed";
#endif
#if defined(SIGBUS)
		case SIGBUS: return "Bus error";
#endif
#if defined(SIGSEGV)
		case SIGSEGV: return "Segmentation fault";
#endif
#if defined(SIGSYS)
		case SIGSYS: return "Invalid system call";
#endif
#if defined(SIGPIPE)
		case SIGPIPE: return "Broken pipe";
#endif
#if defined(SIGALRM)
		case SIGALRM: return "Alarm";
#endif
#if defined(SIGTERM)
		case SIGTERM: return "Terminated";
#endif
#if defined(SIGURG)
		case SIGURG: return "Urgent condition";
#endif
#if defined(SIGSTOP)
		case SIGSTOP: return "Stop";
#endif
#if defined(SIGTSTP)
		case SIGTSTP: return "Stop (tty)";
#endif
#if defined(SIGCONT)
		case SIGCONT: return "Continued stopped process";
#endif
#if defined(SIGCHLD)
		case SIGCHLD: return "Death of child process";
#endif
#if defined(SIGCLD) && SIGCLD != SIGCHLD
		case SIGCLD: return "Death of child process";
#endif
#if defined(SIGTTIN)
		case SIGTTIN: return "Unknown (tty input)";
#endif
#if defined(SIGTTOU)
		case SIGTTOU: return "Unknown (tty output)";
#endif
#if defined(SIGIO) && SIGIO != SIGPOLL
		case SIGIO: return "I/O ready";
#endif
#if defined(SIGIOT) && SIGIOT != SIGABRT
		case SIGIOT: return "IOT instruction";
#endif
#if defined(SIGXCPU)
		case SIGXCPU: return "CPU limit exceeded";
#endif
#if defined(SIGXFSZ)
		case SIGXFSZ: return "File size limit exceeded";
#endif
#if defined(SIGVTALRM)
		case SIGVTALRM: return "Virtual time alarm";
#endif
#if defined(SIGPROF)
		case SIGPROF: return "Profiling time alarm";
#endif
#if defined(SIGWINCH)
		case SIGWINCH: return "Window size changed";
#endif
#if defined(SIGINFO)
		case SIGINFO: return "Status request from keyboard";
#endif
#if defined(SIGUSR1)
		case SIGUSR1: return "User-defined signal 1";
#endif
#if defined(SIGUSR2)
		case SIGUSR2: return "User-defined signal 2";
#endif
		default: break;
	}

	return std::string();
}

/*****************************************************************************/
std::string SubProcess::getSignalNameFromCode(i32 inCode)
{
	if (inCode < 0)
		inCode *= -1;

	switch (inCode)
	{
#if defined(SIGHUP)
		case SIGHUP: return "SIGHUP";
#endif
#if defined(SIGINT)
		case SIGINT: return "SIGINT";
#endif
#if defined(SIGQUIT)
		case SIGQUIT: return "SIGQUIT";
#endif
#if defined(SIGILL)
		case SIGILL: return "SIGILL";
#endif
#if defined(SIGTRAP)
		case SIGTRAP: return "SIGTRAP";
#endif
#if defined(SIGABRT)
		case SIGABRT: return "SIGABRT";
#endif
#if defined(SIGPOLL)
		case SIGPOLL: return "SIGPOLL";
#endif
#if defined(SIGEMT)
		case SIGEMT: return "SIGEMT";
#endif
#if defined(SIGFPE)
		case SIGFPE: return "SIGFPE";
#endif
#if defined(SIGKILL)
		case SIGKILL: return "SIGKILL";
#endif
#if defined(SIGBUS)
		case SIGBUS: return "SIGBUS";
#endif
#if defined(SIGSEGV)
		case SIGSEGV: return "SIGSEGV";
#endif
#if defined(SIGSYS)
		case SIGSYS: return "SIGSYS";
#endif
#if defined(SIGPIPE)
		case SIGPIPE: return "SIGPIPE";
#endif
#if defined(SIGALRM)
		case SIGALRM: return "SIGALRM";
#endif
#if defined(SIGTERM)
		case SIGTERM: return "SIGTERM";
#endif
#if defined(SIGURG)
		case SIGURG: return "SIGURG";
#endif
#if defined(SIGSTOP)
		case SIGSTOP: return "SIGSTOP";
#endif
#if defined(SIGTSTP)
		case SIGTSTP: return "SIGTSTP";
#endif
#if defined(SIGCONT)
		case SIGCONT: return "SIGCONT";
#endif
#if defined(SIGCHLD)
		case SIGCHLD: return "SIGCHLD";
#endif
#if defined(SIGCLD) && SIGCLD != SIGCHLD
		case SIGCLD: return "SIGCHLD";
#endif
#if defined(SIGTTIN)
		case SIGTTIN: return "SIGTTIN";
#endif
#if defined(SIGTTOU)
		case SIGTTOU: return "SIGTTOU";
#endif
#if defined(SIGIO) && SIGIO != SIGPOLL
		case SIGIO: return "SIGIO";
#endif
#if defined(SIGIOT) && SIGIOT != SIGABRT
		case SIGIOT: return "SIGIOT";
#endif
#if defined(SIGXCPU)
		case SIGXCPU: return "SIGXCPU";
#endif
#if defined(SIGXFSZ)
		case SIGXFSZ: return "SIGXFSZ";
#endif
#if defined(SIGVTALRM)
		case SIGVTALRM: return "SIGVTALRM";
#endif
#if defined(SIGPROF)
		case SIGPROF: return "SIGPROF";
#endif
#if defined(SIGWINCH)
		case SIGWINCH: return "SIGWINCH";
#endif
#if defined(SIGINFO)
		case SIGINFO: return "SIGINFO";
#endif
#if defined(SIGUSR1)
		case SIGUSR1: return "SIGUSR1";
#endif
#if defined(SIGUSR2)
		case SIGUSR2: return "SIGUSR2";
#endif
		default: break;
	}

	return std::string();
}

/*****************************************************************************/
ProcessPipe& SubProcess::getFilePipe(const HandleInput inFileNo)
{
	if (inFileNo == FileNo::StdErr)
	{
		return m_err;
	}
	else
	{
		return m_out;
	}
}

/*****************************************************************************/
/*****************************************************************************/
SubProcess::~SubProcess()
{
	close();
}

/*****************************************************************************/
bool SubProcess::operator==(const SubProcess& inProcess)
{
	return m_pid == inProcess.m_pid;
}

/*****************************************************************************/
bool SubProcess::create(const StringList& inCmd, const ProcessOptions& inOptions)
{
#if defined(CHALET_WIN32)
	STARTUPINFOA startupInfo;
	::ZeroMemory(&startupInfo, sizeof(startupInfo));

	PROCESS_INFORMATION processInfo;
	::ZeroMemory(&processInfo, sizeof(processInfo));

	startupInfo.cb = sizeof(startupInfo);
	startupInfo.hStdInput = ::GetStdHandle(FileNo::StdIn);
	startupInfo.hStdOutput = ::GetStdHandle(FileNo::StdOut);
	startupInfo.hStdError = ::GetStdHandle(FileNo::StdErr);
	startupInfo.dwFlags |= STARTF_USESTDHANDLES;

	// m_in.create();

	// input = read
	// output = write

	if (inOptions.stdoutOption == PipeOption::Pipe || inOptions.stdoutOption == PipeOption::Close)
	{
		m_out.create();
		startupInfo.hStdOutput = m_out.m_write;
		m_out.setInheritable(m_out.m_read, false);
	}

	if (inOptions.stderrOption == PipeOption::Pipe || inOptions.stderrOption == PipeOption::Close)
	{
		m_err.create();
		startupInfo.hStdError = m_err.m_write;
		m_err.setInheritable(m_err.m_read, false);
	}
	else if (inOptions.stderrOption == PipeOption::StdOut)
	{
		startupInfo.hStdError = startupInfo.hStdOutput;
	}

	if (inOptions.stdoutOption == PipeOption::StdErr)
	{
		startupInfo.hStdOutput = startupInfo.hStdError;
	}

	{
		const char* cwd = inOptions.cwd.empty() ? nullptr : inOptions.cwd.c_str();
		std::string args = getWindowsArguments(inCmd);

		DWORD processFlags = NORMAL_PRIORITY_CLASS | CREATE_UNICODE_ENVIRONMENT;

		// Disables CTRL+C / CTRL+Break behavior
		if (!inOptions.waitForResult)
		{
			processFlags |= CREATE_NEW_PROCESS_GROUP;
			processFlags |= CREATE_NO_WINDOW;
		}

		BOOL success = ::CreateProcessA(inCmd.front().c_str(),
			args.data(),   // program arguments
			NULL,		   // process security attributes
			NULL,		   // thread security attributes
			TRUE,		   // handles are inherited
			processFlags,  // creation flags
			NULL,		   // environment
			cwd,		   // use parent's current directory
			&startupInfo,  // STARTUPINFO pointer
			&processInfo); // receives PROCESS_INFORMATION

		m_processInfo = processInfo;
		m_pid = processInfo.dwProcessId;

		if (m_out.m_read != m_out.m_write)
			m_out.closeWrite();

		if (m_err.m_read != m_err.m_write)
			m_err.closeWrite();

		if (inOptions.stdoutOption == PipeOption::Close)
			m_out.close();

		if (inOptions.stderrOption == PipeOption::Close)
			m_err.close();

		if (!success)
			return false;
	}

#else

	bool openStdOut = inOptions.stdoutOption == PipeOption::Pipe || inOptions.stdoutOption == PipeOption::Close;
	bool openStdErr = inOptions.stderrOption == PipeOption::Pipe || inOptions.stderrOption == PipeOption::Close;

	bool closeStdIn = inOptions.stdinOption == PipeOption::Close;

	// m_in.create();

	if (openStdOut)
		m_out.create();

	if (openStdErr)
		m_err.create();

	m_pid = fork();
	if (m_pid == -1)
	{
		Diagnostic::error("Couldn't fork process: {}", errno);
		return false;
	}
	else if (m_pid == 0)
	{
		if (!inOptions.cwd.empty())
		{
			if (::chdir(inOptions.cwd.c_str()) != 0)
			{
				Diagnostic::error("Error changing working directory for subprocess: {}", inOptions.cwd);
				return false;
			}
		}

		// m_in.duplicateRead(FileNo::StdIn);
		// m_in.closeWrite();

		if (closeStdIn)
			ProcessPipe::close(FileNo::StdIn);

		if (openStdOut)
		{
			m_out.duplicateWrite(FileNo::StdOut);
			m_out.closeRead();
		}
		// else if (inOptions.stdoutOption == PipeOption::Close)
		// {
		// 	ProcessPipe::close(FileNo::StdOut); // has side effects (see: making dmg on mac)
		// }

		if (openStdErr)
		{
			m_err.duplicateWrite(FileNo::StdErr);
			m_err.closeRead();
		}
		else if (inOptions.stderrOption == PipeOption::StdOut)
		{
			ProcessPipe::duplicate(FileNo::StdOut, FileNo::StdErr);
		}
		// else if (inOptions.stderrOption == PipeOption::Close)
		// {
		// 	ProcessPipe::close(FileNo::StdErr);  // has side effects (see: making dmg on mac)
		// }

		if (inOptions.stdoutOption == PipeOption::StdErr)
		{
			ProcessPipe::duplicate(FileNo::StdErr, FileNo::StdOut);
		}

		CmdPtrArray cmd = getCmdVector(inCmd);
		i32 result = execve(inCmd.front().c_str(), cmd.data(), environ);
		_exit(result == EXIT_SUCCESS ? 0 : errno);
	}

	// UNUSED(m_in);

	// m_in.closeRead();

	if (openStdOut)
		m_out.closeWrite();

	if (openStdErr)
		m_err.closeWrite();
#endif

	if (inOptions.onCreate != nullptr)
	{
		inOptions.onCreate(static_cast<i32>(m_pid));
	}

	return true;
}

/*****************************************************************************/
void SubProcess::close()
{
	m_out.close();
	m_err.close();

#if defined(CHALET_WIN32)
	::CloseHandle(m_processInfo.hProcess);
	::CloseHandle(m_processInfo.hThread);
	::ZeroMemory(&m_processInfo, sizeof(m_processInfo));
#endif

	m_pid = 0;
#if defined(CHALET_MACOS) || defined(CHALET_LINUX)
	m_cwd.clear();
#endif
}

/*****************************************************************************/
bool SubProcess::sendSignal(const SigNum inSignal)
{
	m_killed = true;

#if defined(CHALET_WIN32)
	if (m_pid == 0)
		return true;

	if (inSignal == SigNum::Kill)
	{
		return ::TerminateProcess(m_processInfo.hProcess, 137) == TRUE;
	}
	else if (inSignal == SigNum::Interrupt)
	{
		if (::GenerateConsoleCtrlEvent(CTRL_C_EVENT, m_pid) == FALSE)
		{
			DWORD error = ::GetLastError();
			Diagnostic::error("GenerateConsoleCtrlEvent CTRL_C_EVENT error: {}", error);
			return false;
		}
	}
	else
	{
		if (::GenerateConsoleCtrlEvent(CTRL_BREAK_EVENT, m_pid) == FALSE)
		{
			DWORD error = ::GetLastError();
			Diagnostic::error("GenerateConsoleCtrlEvent CTRL_BREAK_EVENT error: {}", error);
			return false;
		}
	}

#else
	if (m_pid == -1)
		return true;

	if (::kill(m_pid, static_cast<i32>(inSignal)) < 0)
	{
		// bool result = errno == ESRCH;
		// if (!result)
		// {
		// 	Diagnostic::error("Error shutting down process: {}", m_pid);
		// }
		// return result;
	}
#endif

	return true;
}

/*****************************************************************************/
bool SubProcess::terminate()
{
	return sendSignal(SigNum::Terminate);
}

/*****************************************************************************/
bool SubProcess::kill()
{
	return sendSignal(SigNum::Kill);
}

/*****************************************************************************/
void SubProcess::read(HandleInput inFileNo, const ProcessOptions::PipeFunc& onRead)
{
	auto& pipe = getFilePipe(inFileNo);
#if defined(CHALET_WIN32)
	DWORD bytesRead = 0;
#else
	ssize_t bytesRead = std::numeric_limits<ssize_t>::max();
#endif
	auto bufferData = m_DataBuffer.data();
	size_t bufferSize = m_DataBuffer.size();
	while (true)
	{
		if (m_killed)
			break;

#if defined(CHALET_WIN32)
		bool result = ::ReadFile(pipe.m_read, static_cast<LPVOID>(bufferData), static_cast<DWORD>(bufferSize), static_cast<LPDWORD>(&bytesRead), nullptr) == TRUE;
		if (!result)
			bytesRead = 0;
#else
		bytesRead = ::read(pipe.m_read, bufferData, bufferSize);
#endif
		if (bytesRead > 0)
		{
			if (onRead != nullptr)
				onRead(std::string(bufferData, bytesRead));
		}
		else
			break;
	}
}

}
