/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Process/SubProcess.hpp"

namespace chalet
{
/*****************************************************************************/
template <size_t Size>
void SubProcess::read(HandleInput inFileNo, std::array<char, Size>& inBuffer, const ProcessOptions::PipeFunc& onRead)
{
	auto& pipe = getFilePipe(inFileNo);
#if defined(CHALET_WIN32)
	DWORD bytesRead = 0;
#else
	ssize_t bytesRead = 0;
#endif
	size_t bufferSize = inBuffer.size();
	while (true)
	{
		if (m_killed)
			break;

#if defined(CHALET_WIN32)
		bool result = ::ReadFile(pipe.m_read, static_cast<LPVOID>(inBuffer.data()), static_cast<DWORD>(bufferSize), static_cast<LPDWORD>(&bytesRead), nullptr) == TRUE;
		if (!result)
			bytesRead = 0;
#else
		bytesRead = ::read(pipe.m_read, inBuffer.data(), bufferSize);
#endif
		if (bytesRead > 0)
		{
			if (onRead != nullptr)
				onRead(std::string(inBuffer.data(), bytesRead));
		}
		else
			break;
	}
}
}
