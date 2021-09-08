/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Process/RunningProcess.hpp"

#if defined(CHALET_WIN32)
#else
	#include <sys/wait.h>
	#include <unistd.h>
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
	for (std::size_t i = 0; i < inCmd.size(); ++i)
	{
		if (i > 0)
			args += ' ';

		args += escapeShellArgument(inCmd[i]);
	}

	return args;
}
}

/*****************************************************************************/
int RunningProcess::waitForResult()
{
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
	return static_cast<int>(exitCode);
}
#else
/*****************************************************************************/
int RunningProcess::waitForResult()
{
	int exitCode;
	while (true)
	{
		ProcessID child = ::waitpid(m_pid, &exitCode, 0);
		if (child == -1 && errno == EINTR)
			continue;

		break;
	}

	int result = getReturnCode(exitCode);

	close();
	return result;
}

/*****************************************************************************/
int RunningProcess::getReturnCode(const int inExitCode)
{
	if (WIFEXITED(inExitCode))
		return WEXITSTATUS(inExitCode);
	else if (WIFSIGNALED(inExitCode))
		return -WTERMSIG(inExitCode);
	else
		return 1;
}

/*****************************************************************************/
RunningProcess::CmdPtrArray RunningProcess::getCmdVector(const StringList& inCmd)
{
	CmdPtrArray cmd;

	cmd.reserve(inCmd.size() + 1);
	for (std::size_t i = 0; i < inCmd.size(); ++i)
	{
		cmd.emplace_back(const_cast<char*>(inCmd.at(i).data()));
	}
	cmd.push_back(nullptr);

	return cmd;
}

#endif

/*****************************************************************************/
ProcessPipe& RunningProcess::getFilePipe(const HandleInput inFileNo)
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
RunningProcess::~RunningProcess()
{
	close();
}

/*****************************************************************************/
bool RunningProcess::operator==(const RunningProcess& inProcess)
{
	return m_pid == inProcess.m_pid;
}

/*****************************************************************************/
bool RunningProcess::create(const StringList& inCmd, const ProcessOptions& inOptions)
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

		DWORD processFlags = HIGH_PRIORITY_CLASS | CREATE_UNICODE_ENVIRONMENT;
		/*if (m_newProcessGroup)
		{
			processFlags |= CREATE_NEW_PROCESS_GROUP;
		}*/

		BOOL success = ::CreateProcessA(inCmd.front().c_str(),
			const_cast<LPSTR>(args.c_str()), // program arguments
			NULL,							 // process security attributes
			NULL,							 // thread security attributes
			TRUE,							 // handles are inherited
			processFlags,					 // creation flags
			NULL,							 // environment
			cwd,							 // use parent's current directory
			&startupInfo,					 // STARTUPINFO pointer
			&processInfo);					 // receives PROCESS_INFORMATION

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
		{
			DWORD error = ::GetLastError();
			Diagnostic::error("CreateProcess error: {}", error);
			return false;
		}
	}

#else
	m_cmd = getCmdVector(inCmd);

	bool openStdOut = inOptions.stdoutOption == PipeOption::Pipe;
	bool openStdErr = inOptions.stderrOption == PipeOption::Pipe;

	// m_in.create();

	if (openStdOut)
		m_out.create();

	if (openStdErr)
		m_err.create();

	m_pid = fork();
	if (m_pid == -1)
	{
		Diagnostic::error("can't fork process. Error: {}", errno);
		return false;
	}
	else if (m_pid == 0)
	{
		if (!inOptions.cwd.empty())
			::chdir(inOptions.cwd.c_str());

		// m_in.duplicateRead(FileNo::StdIn);
		// m_in.closeWrite();

		ProcessPipe::close(FileNo::StdIn);

		if (openStdOut)
		{
			m_out.duplicateWrite(FileNo::StdOut);
			m_out.closeRead();
		}
		else if (inOptions.stdoutOption == PipeOption::Close)
		{
			ProcessPipe::close(FileNo::StdOut);
		}

		if (openStdErr)
		{
			m_err.duplicateWrite(FileNo::StdErr);
			m_err.closeRead();
		}
		else if (inOptions.stderrOption == PipeOption::StdOut)
		{
			ProcessPipe::duplicate(FileNo::StdOut, FileNo::StdErr);
		}
		else if (inOptions.stderrOption == PipeOption::Close)
		{
			ProcessPipe::close(FileNo::StdErr);
		}

		if (inOptions.stdoutOption == PipeOption::StdErr)
		{
			ProcessPipe::duplicate(FileNo::StdErr, FileNo::StdOut);
		}

		execve(inCmd.front().c_str(), m_cmd.data(), environ);
		_exit(0);
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
		inOptions.onCreate(static_cast<int>(m_pid));
	}

	return true;
}

/*****************************************************************************/
void RunningProcess::close()
{
	m_out.close();
	m_err.close();

#if defined(CHALET_WIN32)
	::CloseHandle(m_processInfo.hProcess);
	::CloseHandle(m_processInfo.hThread);
	::ZeroMemory(&m_processInfo, sizeof(m_processInfo));
#endif

	m_pid = 0;
	m_cmd.clear();
#if !defined(CHALET_WIN32)
	m_cwd.clear();
#endif
}

/*****************************************************************************/
bool RunningProcess::sendSignal(const SigNum inSignal)
{
#if defined(CHALET_WIN32)
	if (m_pid == 0)
		return false;

	m_killed = true;

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
		return false;

	m_killed = true;

	if (::kill(m_pid, static_cast<int>(inSignal)) != 0)
	{
		Diagnostic::error("Error shutting down process: {}", m_pid);
		return false;
	}
#endif

	return true;
}

/*****************************************************************************/
bool RunningProcess::terminate()
{
	return sendSignal(SigNum::Terminate);
}

/*****************************************************************************/
bool RunningProcess::kill()
{
	return sendSignal(SigNum::Kill);
}

}
