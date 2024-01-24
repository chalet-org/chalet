/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include <array>

#include "Process/ProcessOptions.hpp"
#include "Process/ProcessPipe.hpp"
#include "Process/ProcessTypes.hpp"
#include "Process/SigNum.hpp"

namespace chalet
{
class SubProcess
{
	using CmdPtrArray = std::vector<char*>;
#if defined(CHALET_WIN32)
	using HandleInput = DWORD;
#else
	using HandleInput = PipeHandle;
#endif

public:
	SubProcess() = default;
	CHALET_DISALLOW_COPY_MOVE(SubProcess);
	~SubProcess();
	bool operator==(const SubProcess& rhs);

	static std::string getErrorMessageFromCode(const i32 inCode);
	static std::string getErrorMessageFromSignalRaised(const i32 inCode);
	static std::string getSignalNameFromCode(i32 inCode);

	bool create(const StringList& inCmd, const ProcessOptions& inOptions);
	void close();

	i32 waitForResult();
	bool sendSignal(const SigNum inSignal);
	bool terminate();
	bool kill();

	void read(HandleInput inFileNo, const ProcessOptions::PipeFunc& onRead = nullptr);

private:
#if defined(CHALET_MACOS) || defined(CHALET_LINUX)
	i32 getReturnCode(const i32 inExitCode);
	CmdPtrArray getCmdVector(const StringList& inCmd);
#endif
	ProcessPipe& getFilePipe(const HandleInput inFileNo);

	static constexpr size_t kDataBufferSize = 256;
	std::array<char, kDataBufferSize> m_DataBuffer;

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
