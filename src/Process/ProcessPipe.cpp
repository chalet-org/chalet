/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Process/ProcessPipe.hpp"

#if defined(CHALET_WIN32)
#else
	#include <fcntl.h>
#endif

namespace chalet
{

/*****************************************************************************/
ProcessPipe::~ProcessPipe()
{
	closeRead();
	closeWrite();
}

#if defined(CHALET_MACOS) || defined(CHALET_LINUX)
/*****************************************************************************/
void ProcessPipe::duplicate(PipeHandle oldFd, PipeHandle newFd)
{
	if (::dup2(oldFd, newFd) == -1)
	{
		Diagnostic::error("Error duplicating file descripter");
		return;
	}
}
#endif

/*****************************************************************************/
void ProcessPipe::setInheritable(PipeHandle inHandle, const bool inInherits)
{
	if (inHandle == kInvalidPipe)
		return;

#if defined(CHALET_WIN32)
	bool result = ::SetHandleInformation(inHandle, HANDLE_FLAG_INHERIT, inInherits ? HANDLE_FLAG_INHERIT : 0) == TRUE;
	if (!result)
	{
		Diagnostic::error("Error calling SetHandleInformation");
	}
#else
	i32 flags = ::fcntl(inHandle, F_GETFD);
	if (flags < 0)
	{
		Diagnostic::error("Error calling fcntl");
		return;
	}

	if (inInherits)
		flags &= ~FD_CLOEXEC;
	else
		flags |= FD_CLOEXEC;

	i32 result = ::fcntl(inHandle, F_SETFD, flags);
	if (result < -1)
	{
		Diagnostic::error("Error calling fcntl");
	}
#endif
}

/*****************************************************************************/
void ProcessPipe::close(PipeHandle newFd)
{
	if (newFd == kInvalidPipe)
		return;

#if defined(CHALET_WIN32)
	bool result = ::CloseHandle(newFd) == TRUE;
	if (!result)
#else
	if (::close(newFd) != 0)
#endif
	{
		Diagnostic::error("Error closing pipe");
	}
}

/*****************************************************************************/
/*****************************************************************************/
void ProcessPipe::create(const bool inInheritable)
{
#if defined(CHALET_WIN32)
	SECURITY_ATTRIBUTES security;
	::ZeroMemory(&security, sizeof(security));

	security.nLength = sizeof(SECURITY_ATTRIBUTES);
	security.bInheritHandle = inInheritable;
	security.lpSecurityDescriptor = NULL;

	bool result = ::CreatePipe(&m_read, &m_write, &security, 0) == TRUE;
	if (!result)
	{
		m_read = m_write = kInvalidPipe;
		Diagnostic::error("Error opening pipe");
	}
#else
	if (::pipe(reinterpret_cast<i32*>(this)) != 0)
	{
		Diagnostic::error("Error opening pipe");
		return;
	}

	if (!inInheritable)
	{
		setInheritable(m_read, false);
		setInheritable(m_write, false);
	}
#endif
}

#if defined(CHALET_MACOS) || defined(CHALET_LINUX)
/*****************************************************************************/
void ProcessPipe::duplicateRead(PipeHandle newFd)
{
	if (::dup2(m_read, newFd) == -1)
	{
		Diagnostic::error("Error duplicating read file descripter");
	}
}

/*****************************************************************************/
void ProcessPipe::duplicateWrite(PipeHandle newFd)
{
	if (::dup2(m_write, newFd) == -1)
	{
		Diagnostic::error("Error duplicating write file descripter");
	}
}
#endif

/*****************************************************************************/
void ProcessPipe::closeRead()
{
	if (m_read == kInvalidPipe)
		return;

#if defined(CHALET_WIN32)
	bool result = ::CloseHandle(m_read) == TRUE;
	if (!result)
#else
	if (::close(m_read) != 0)
#endif
	{
		Diagnostic::error("Error closing read pipe");
	}
	m_read = kInvalidPipe;
}

/*****************************************************************************/
void ProcessPipe::closeWrite()
{
	if (m_write == kInvalidPipe)
		return;

#if defined(CHALET_WIN32)
	bool result = ::CloseHandle(m_write) == TRUE;
	if (!result)
#else
	if (::close(m_write) != 0)
#endif
	{
		Diagnostic::error("Error closing write pipe");
	}
	m_write = kInvalidPipe;
}

/*****************************************************************************/
void ProcessPipe::close()
{
	closeRead();
	closeWrite();
}
}
