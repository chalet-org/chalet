/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Process/ProcessPipe.hpp"

namespace chalet
{
/*****************************************************************************/
void ProcessPipe::duplicate(PipeHandle oldFd, PipeHandle newFd)
{
	if (::dup2(oldFd, newFd) == -1)
	{
		CHALET_THROW(std::runtime_error("Error duplicating file descripter"));
	}
}

void ProcessPipe::close(PipeHandle newFd)
{
	if (newFd == kInvalidPipe)
		return;

	if (::close(newFd) != 0)
	{
		CHALET_THROW(std::runtime_error("Error closing read pipe"));
	}
}

/*****************************************************************************/
void ProcessPipe::openPipe()
{
	if (::pipe(reinterpret_cast<int*>(this)) != 0)
	{
		CHALET_THROW(std::runtime_error("Error opening pipe"));
	}

	// TODO: pipe_set_inheritable false
}

/*****************************************************************************/
void ProcessPipe::duplicateRead(PipeHandle newFd)
{
	if (::dup2(m_read, newFd) == -1)
	{
		CHALET_THROW(std::runtime_error("Error duplicating read file descripter"));
	}
}

/*****************************************************************************/
void ProcessPipe::duplicateWrite(PipeHandle newFd)
{
	if (::dup2(m_write, newFd) == -1)
	{
		CHALET_THROW(std::runtime_error("Error duplicating write file descripter"));
	}
}

/*****************************************************************************/
void ProcessPipe::closeRead()
{
	if (m_read == kInvalidPipe)
		return;

	if (::close(m_read) != 0)
	{
		CHALET_THROW(std::runtime_error("Error closing read pipe"));
	}
	m_read = kInvalidPipe;
}

/*****************************************************************************/
void ProcessPipe::closeWrite()
{
	if (m_write == kInvalidPipe)
		return;

	if (::close(m_write) != 0)
	{
		CHALET_THROW(std::runtime_error("Error closing write pipe"));
	}
	m_write = kInvalidPipe;
}
}
