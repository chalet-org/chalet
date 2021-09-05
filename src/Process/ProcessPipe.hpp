/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_PROCESS_PIPE_HPP
#define CHALET_PROCESS_PIPE_HPP

#include "Process/ProcessTypes.hpp"

namespace chalet
{
class ProcessPipe
{
	friend class OpenProcess;

public:
	static void duplicate(PipeHandle oldFd, PipeHandle newFd);
	static void close(PipeHandle newFd);

	void openPipe();
	void duplicateRead(PipeHandle newFd);
	void duplicateWrite(PipeHandle newFd);
	void closeRead();
	void closeWrite();

private:
	PipeHandle m_read = kInvalidPipe;
	PipeHandle m_write = kInvalidPipe;
};
}

#endif // CHALET_PROCESS_PIPE_HPP
