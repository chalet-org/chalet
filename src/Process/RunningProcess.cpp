/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Process/RunningProcess.hpp"

namespace chalet
{
/*****************************************************************************/
int RunningProcess::getReturnCode(const int inExitCode)
{
#if defined(CHALET_WIN32)
	return inExitCode;
#else
	if (WIFEXITED(inExitCode))
		return WEXITSTATUS(inExitCode);
	else if (WIFSIGNALED(inExitCode))
		return -WTERMSIG(inExitCode);
	else
		return 1;
#endif
}

/*****************************************************************************/
int RunningProcess::waitForResult(const ProcessID inPid, CmdPtrArray& inCmd)
{
	int exitCode;
	while (true)
	{
		ProcessID child = ::waitpid(inPid, &exitCode, 0);
		if (child == -1 && errno == EINTR)
			continue;

		break;
	}

	inCmd.clear();

	return getReturnCode(exitCode);
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

/*****************************************************************************/
int RunningProcess::createWithoutPipes(const StringList& inCmd, const std::string& inCwd)
{
	std::string cwd;
	if (!inCwd.empty())
	{
		cwd = std::filesystem::current_path().string();
		std::filesystem::current_path(inCwd);
	}

	CmdPtrArray cmd = getCmdVector(inCmd);

	ProcessID pid = fork();
	if (pid == -1)
	{
		CHALET_THROW(std::runtime_error(fmt::format("can't fork process. Error:", errno)));
		return -1;
	}
	else if (pid == 0)
	{
		execve(inCmd.front().c_str(), cmd.data(), environ);
		_exit(0);
	}

	if (!cwd.empty())
		std::filesystem::current_path(cwd);

	cmd.clear();

	return waitForResult(pid, cmd);
}

/*****************************************************************************/
/*****************************************************************************/
RunningProcess::~RunningProcess()
{
	close();
}

/*****************************************************************************/
ProcessPipe& RunningProcess::getFilePipe(const PipeHandle inFileNo)
{
#if defined(CHALET_WIN32)
#else
	switch (inFileNo)
	{
		case FileNo::StdErr: return m_err;
		case FileNo::StdIn: // return m_in;
		default: break;
	}
#endif

	return m_out;
}

/*****************************************************************************/
void RunningProcess::close()
{
	m_out.closeRead();
	m_out.closeWrite();
	m_err.closeRead();
	m_err.closeWrite();

#if defined(CHALET_WIN32)
	CloseHandle(m_processInfo.hProcess);
	CloseHandle(m_processInfo.hThread);
	ZeroMemory(&m_processInfo, sizeof(m_processInfo));
#endif

	m_pid = 0;
	m_cmd.clear();
	m_cwd.clear();
}

/*****************************************************************************/
bool RunningProcess::create(const StringList& inCmd, const ProcessOptions& inOptions)
{
	if (!inOptions.cwd.empty())
	{
		m_cwd = std::filesystem::current_path().string();
		std::filesystem::current_path(inOptions.cwd);
	}

	m_cmd = getCmdVector(inCmd);

	bool openStdOut = inOptions.stdoutOption == PipeOption::Pipe;
	bool openStdErr = inOptions.stderrOption == PipeOption::Pipe;

	// m_in.openPipe();

	if (openStdOut)
		m_out.openPipe();

	if (openStdErr)
		m_err.openPipe();

	m_pid = fork();
	if (m_pid == -1)
	{
		CHALET_THROW(std::runtime_error(fmt::format("can't fork process. Error:", errno)));
		return false;
	}
	else if (m_pid == 0)
	{
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

	if (!m_cwd.empty())
	{
		std::filesystem::current_path(m_cwd);
		m_cwd.clear();
	}

	if (openStdOut)
		m_out.closeWrite();

	if (openStdErr)
		m_err.closeWrite();

	if (inOptions.onCreate != nullptr)
	{
		inOptions.onCreate(static_cast<int>(m_pid));
	}

	return true;
}

/*****************************************************************************/
int RunningProcess::waitForResult()
{
	return waitForResult(m_pid, m_cmd);
}

/*****************************************************************************/
bool RunningProcess::sendSignal(const SigNum inSignal)
{
	if (m_pid == -1)
		return false;

	m_killed = true;

#if defined(CHALET_WIN32)
	if (inSignal == SigNum::Kill)
	{
	}
#else
	return ::kill(m_pid, static_cast<int>(inSignal)) == 0;
#endif
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
