/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_PROCESS_HPP
#define CHALET_PROCESS_HPP

#include "Process/ProcessOptions.hpp"
#include "Process/ProcessPipe.hpp"
#include "Process/ProcessTypes.hpp"
#include "Process/SigNum.hpp"

namespace chalet
{
class Process
{
	using CmdPtrArray = std::vector<char*>;
#if defined(CHALET_WIN32)
	using HandleInput = DWORD;
#else
	using HandleInput = PipeHandle;
#endif

public:
	Process() = default;
	CHALET_DISALLOW_COPY_MOVE(Process);
	~Process();
	bool operator==(const Process& rhs);

	static std::string getErrorMessageFromCode(const int inCode);
	static std::string getErrorMessageFromSignalRaised(const int inCode);
	static std::string getSignalNameFromCode(int inCode);

	bool create(const StringList& inCmd, const ProcessOptions& inOptions);
	void close();

	int waitForResult();
	bool sendSignal(const SigNum inSignal);
	bool terminate();
	bool kill();

	template <std::size_t Size>
	void read(HandleInput inFileNo, std::array<char, Size>& inBuffer, const std::uint8_t inBufferSize, const ProcessOptions::PipeFunc& onRead = nullptr);

private:
#if defined(CHALET_MACOS) || defined(CHALET_LINUX)
	int getReturnCode(const int inExitCode);
	CmdPtrArray getCmdVector(const StringList& inCmd);
#endif
	ProcessPipe& getFilePipe(const HandleInput inFileNo);

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

#include "Process/Process.inl"

#endif // CHALET_PROCESS_HPP
