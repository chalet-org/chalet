/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Utility/Subprocess2.hpp"

#include <array>

#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

#include "Libraries/Format.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/OSTerminal.hpp"
#include "Terminal/Output.hpp"

extern char** environ;

namespace chalet
{
namespace
{
/*****************************************************************************/
enum class FileNo : int
{
	StdIn = STDIN_FILENO,
	StdOut = STDOUT_FILENO,
	StdErr = STDERR_FILENO
};

enum class SigNum : int
{
	HangUp = 1,
	Interrupt = SIGINT,
	Quit = 3,
	IllegalInstruction = SIGILL,
	Trap = 5,
	Abort = SIGABRT,
	// SIGIOT = 6,
	// SIGBUS = 7,
	FloatingPointException = SIGFPE,
	Kill = 9,
	PSIGUSR1 = 10,
	SegmentationViolation = SIGSEGV,
	PSIGUSR2 = 12,
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

/*****************************************************************************/
class Pipe
{
	friend class OpenProcess;

	int m_read;
	int m_write;

public:
	inline void openPipe()
	{
		if (::pipe(reinterpret_cast<int*>(this)) != 0)
		{
			CHALET_THROW(std::runtime_error("Error opening pipe"));
		}
	}

	inline void duplicateRead(const FileNo newFd)
	{
		if (::dup2(m_read, static_cast<int>(newFd)) == -1)
		{
			CHALET_THROW(std::runtime_error("Error duplicating read file descripter"));
		}
	}

	inline void duplicateWrite(const FileNo newFd)
	{
		if (::dup2(m_write, static_cast<int>(newFd)) == -1)
		{
			CHALET_THROW(std::runtime_error("Error duplicating write file descripter"));
		}
	}

	static void duplicate(const FileNo oldFd, const FileNo newFd)
	{
		if (::dup2(static_cast<int>(oldFd), static_cast<int>(newFd)) == -1)
		{
			CHALET_THROW(std::runtime_error("Error duplicating file descripter"));
		}
	}

	static void close(const FileNo newFd)
	{
		if (::close(static_cast<int>(newFd)) != 0)
		{
			CHALET_THROW(std::runtime_error("Error closing read pipe"));
		}
	}

	inline void closeRead()
	{
		if (::close(m_read) != 0)
		{
			CHALET_THROW(std::runtime_error("Error closing read pipe"));
		}
	}

	inline void closeWrite()
	{
		if (::close(m_write) != 0)
		{
			CHALET_THROW(std::runtime_error("Error closing write pipe"));
		}
	}
};

/*****************************************************************************/
class OpenProcess
{
	std::vector<char*> m_cmd;
	std::string m_cwd;

	// Pipe m_in;
	Pipe m_out;
	Pipe m_err;

	bool m_killed = false;

	static int getReturnCode(const int inExitCode)
	{
		if (WIFEXITED(inExitCode))
			return WEXITSTATUS(inExitCode);
		else if (WIFSIGNALED(inExitCode))
			return -WTERMSIG(inExitCode);
		else
			return 1;
	}

	static int waitForResult(const pid_t inPid, std::vector<char*>& inCmd)
	{
		int exitCode;
		while (true)
		{
			pid_t child = ::waitpid(inPid, &exitCode, 0);
			if (child == -1 && errno == EINTR)
				continue;

			break;
		}

		inCmd.clear();

		return getReturnCode(exitCode);
	}

	static std::vector<char*> getCmdVector(const StringList& inCmd)
	{
		std::vector<char*> cmd;

		cmd.reserve(inCmd.size() + 1);
		for (std::size_t i = 0; i < inCmd.size(); ++i)
		{
			cmd.emplace_back(const_cast<char*>(inCmd.at(i).data()));
		}
		cmd.push_back(nullptr);

		return cmd;
	}

	Pipe& getFilePipe(const FileNo inFileNo)
	{
		switch (inFileNo)
		{
			case FileNo::StdErr:
				return m_err;
			case FileNo::StdIn:
				// return m_in;
			default:
				break;
		}

		return m_out;
	}

public:
	pid_t m_pid = -1;

	static int createWithoutPipes(const StringList& inCmd, const std::string& inCwd)
	{
		std::string cwd;
		if (!inCwd.empty())
		{
			cwd = std::filesystem::current_path().string();
			std::filesystem::current_path(inCwd);
		}

		std::vector<char*> cmd = getCmdVector(inCmd);

		pid_t pid = fork();
		if (pid == -1)
		{
			CHALET_THROW(std::runtime_error(fmt::format("can't fork process. Error:", errno)));
			return -1;
		}
		else if (pid == 0)
		{
			execve(inCmd.front().c_str(), cmd.data(), NULL);
			_exit(0);
		}

		if (!cwd.empty())
			std::filesystem::current_path(cwd);

		cmd.clear();

		return waitForResult(pid, cmd);
	}

	bool create(const StringList& inCmd, const SubprocessOptions& inOptions)
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

			Pipe::close(FileNo::StdIn);

			if (openStdOut)
			{
				m_out.duplicateWrite(FileNo::StdOut);
				m_out.closeRead();
			}
			else if (inOptions.stdoutOption == PipeOption::Close)
			{
				Pipe::close(FileNo::StdOut);
			}

			if (openStdErr)
			{
				m_err.duplicateWrite(FileNo::StdErr);
				m_err.closeRead();
			}
			else if (inOptions.stderrOption == PipeOption::StdOut)
			{
				Pipe::duplicate(FileNo::StdOut, FileNo::StdErr);
			}
			else if (inOptions.stderrOption == PipeOption::Close)
			{
				Pipe::close(FileNo::StdErr);
			}

			if (inOptions.stdoutOption == PipeOption::StdErr)
			{
				Pipe::duplicate(FileNo::StdErr, FileNo::StdOut);
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

	template <class T, size_t Size>
	inline void read(const FileNo inFileNo, std::array<T, Size>& inBuffer, const SubprocessOptions::PipeFunc& onRead = nullptr)
	{
		if (onRead != nullptr)
		{
			auto& pipe = getFilePipe(inFileNo);
			ssize_t bytesRead = 0;
			while (true)
			{
				if (m_killed)
				{
					std::cout << std::endl;
					break;
				}

				bytesRead = ::read(pipe.m_read, inBuffer.data(), inBuffer.size());
				if (bytesRead > 0)
					onRead(std::string_view(inBuffer.data(), bytesRead));
				else
					break;
			}
		}
	}

	inline int waitForResult()
	{
		return waitForResult(m_pid, m_cmd);
	}

	bool sendSignal(const SigNum inSignal)
	{
		if (m_pid == -1)
			return false;

		m_killed = true;

		return ::kill(m_pid, static_cast<int>(inSignal)) == 0;
	}

	bool terminate()
	{
		return sendSignal(SigNum::Terminate);
	}

	bool kill()
	{
		return sendSignal(SigNum::Kill);
	}
};

std::vector<OpenProcess*> s_procesess;
std::atomic<int> s_lastErrorCode = 0;
bool s_initialized = false;

/*****************************************************************************/
void removeProcess(const OpenProcess& inProcess)
{
	auto it = s_procesess.end();
	while (it != s_procesess.begin())
	{
		--it;
		OpenProcess* process = (*it);
		if (process->m_pid == inProcess.m_pid)
		{
			it = s_procesess.erase(it);
			return;
		}
	}

	if (s_procesess.empty())
		OSTerminal::reset();
}

/*****************************************************************************/
void subProcessSignalHandler(int inSignal)
{
	auto it = s_procesess.end();
	while (it != s_procesess.begin())
	{
		--it;
		OpenProcess* process = (*it);

		if (static_cast<SigNum>(inSignal) == SigNum::Terminate)
			process->terminate();
		else
			process->sendSignal(static_cast<SigNum>(inSignal));

		it = s_procesess.erase(it);
	}

	OSTerminal::reset();
}
}

/*****************************************************************************/
int Subprocess2::run(const StringList& inCmd, SubprocessOptions&& inOptions)
{
	CHALET_TRY
	{
		if (!s_initialized)
		{
			::signal(SIGINT, subProcessSignalHandler);
			::signal(SIGTERM, subProcessSignalHandler);
			::signal(SIGABRT, subProcessSignalHandler);
			s_initialized = true;
		}

		if (inCmd.empty())
			return 0;

		if (inOptions.stdoutOption == PipeOption::StdOut && inOptions.stderrOption == PipeOption::StdErr)
		{
			return OpenProcess::createWithoutPipes(inCmd, inOptions.cwd);
		}
		else
		{
			OpenProcess process;
			if (!process.create(inCmd, inOptions))
				return 1;

			s_procesess.push_back(&process);

			static std::array<char, 256> buffer{ 0 };

			if (inOptions.stdoutOption == PipeOption::Pipe)
				process.read(FileNo::StdOut, buffer, inOptions.onStdOut);

			if (inOptions.stderrOption == PipeOption::Pipe)
				process.read(FileNo::StdErr, buffer, inOptions.onStdErr);

			int result = process.waitForResult();

			removeProcess(process);
			s_lastErrorCode = result;

			return result;
		}
	}
	CHALET_CATCH(const std::exception& err)
	{
		CHALET_EXCEPT_ERROR("subprocess2 error: {}", err.what());
		return -1;
	}
}

/*****************************************************************************/
int Subprocess2::getLastExitCode()
{
	return s_lastErrorCode;
}

/*****************************************************************************/
void Subprocess2::haltAllProcesses(const int inSignal)
{
	subProcessSignalHandler(inSignal);
}
}
