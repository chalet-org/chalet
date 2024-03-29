/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include "Process/ProcessTypes.hpp"

namespace chalet
{
class ProcessPipe
{
	friend class SubProcess;

public:
	ProcessPipe() = default;
	CHALET_DISALLOW_COPY_MOVE(ProcessPipe);
	~ProcessPipe();

#if defined(CHALET_MACOS) || defined(CHALET_LINUX)
	static void duplicate(PipeHandle oldFd, PipeHandle newFd);
#endif
	static void setInheritable(PipeHandle inHandle, const bool inInherits);
	static void close(PipeHandle newFd);

	void create(const bool inInheritable = true);
#if defined(CHALET_MACOS) || defined(CHALET_LINUX)
	void duplicateRead(PipeHandle newFd);
	void duplicateWrite(PipeHandle newFd);
#endif
	void closeRead();
	void closeWrite();
	void close();

private:
	PipeHandle m_read = kInvalidPipe;
	PipeHandle m_write = kInvalidPipe;
};
}
