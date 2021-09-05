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

public:
	RunningProcess() = default;
	CHALET_DISALLOW_COPY_MOVE(RunningProcess);
	~RunningProcess();

	static int createWithoutPipes(const StringList& inCmd, const std::string& inCwd);

	void close();
	bool create(const StringList& inCmd, const ProcessOptions& inOptions);

	template <size_t Size>
	void read(PipeHandle inFileNo, std::array<char, Size>& inBuffer, const std::uint8_t inBufferSize, const ProcessOptions::PipeFunc& onRead = nullptr);

	int waitForResult();

	bool sendSignal(const SigNum inSignal);

	bool terminate();
	bool kill();

	ProcessID m_pid = 0;

private:
	static int getReturnCode(const int inExitCode);
	static int waitForResult(const ProcessID inPid, CmdPtrArray& inCmd);
	static CmdPtrArray getCmdVector(const StringList& inCmd);

	ProcessPipe& getFilePipe(const PipeHandle inFileNo);

	CmdPtrArray m_cmd;
	std::string m_cwd;
#if defined(CHALET_WIN32)
	PROCESS_INFORMATION m_processInfo{ 0, 0, 0, 0 };
#endif

	// ProcessPipe m_in;
	ProcessPipe m_out;
	ProcessPipe m_err;

	bool m_killed = false;
};
}

#include "Process/RunningProcess.inl"

#endif // CHALET_RUNNING_PROCESS_HPP
