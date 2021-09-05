/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_RUNNING_PROCESS_HPP
#define CHALET_RUNNING_PROCESS_HPP

#include "Process/ProcessOptions.hpp"
#include "Process/ProcessPipe.hpp"
#include "Process/ProcessTypes.hpp"
#include "Process/SigNum.hpp"

namespace chalet
{
class RunningProcess
{
	using CmdPtrArray = std::vector<char*>;
#if defined(CHALET_WIN32)
	using HandleInput = DWORD;
#else
	using HandleInput = PipeHandle;
#endif

public:
	RunningProcess() = default;
	CHALET_DISALLOW_COPY_MOVE(RunningProcess);
	~RunningProcess();
	bool operator==(const RunningProcess& rhs);

	bool create(const StringList& inCmd, const ProcessOptions& inOptions);
	void close();

	int waitForResult();
	bool sendSignal(const SigNum inSignal);
	bool terminate();
	bool kill();

	template <size_t Size>
	void read(HandleInput inFileNo, std::array<char, Size>& inBuffer, const std::uint8_t inBufferSize, const ProcessOptions::PipeFunc& onRead = nullptr);

private:
#if defined(CHALET_WIN32)
	static int waitForResult(PROCESS_INFORMATION& inProcessInfo);
#else
	static int waitForResult(const ProcessID inPid);
	static int getReturnCode(const int inExitCode);
	static CmdPtrArray getCmdVector(const StringList& inCmd);
#endif
	ProcessPipe& getFilePipe(const HandleInput inFileNo);

	CmdPtrArray m_cmd;
#if defined(CHALET_WIN32)
	PROCESS_INFORMATION m_processInfo{ 0, 0, 0, 0 };
#else
	std::string m_cwd;
#endif

	// ProcessPipe m_in;
	ProcessPipe m_out;
	ProcessPipe m_err;

	ProcessID m_pid = 0;

	bool m_killed = false;
};
}

#include "Process/RunningProcess.inl"

#endif // CHALET_RUNNING_PROCESS_HPP
