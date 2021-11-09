/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Process/Process.hpp"

namespace chalet
{
/*****************************************************************************/
template <std::size_t Size>
void Process::read(HandleInput inFileNo, std::array<char, Size>& inBuffer, const std::uint8_t inBufferSize, const ProcessOptions::PipeFunc& onRead)
{
	auto& pipe = getFilePipe(inFileNo);
#if defined(CHALET_WIN32)
	DWORD bytesRead = 0;
#else
	ssize_t bytesRead = 0;
#endif
	std::size_t bufferSize = inBufferSize > 0 ? static_cast<std::size_t>(inBufferSize) : inBuffer.size();
	while (true)
	{
		if (m_killed)
		{
			std::cout << std::endl;
			break;
		}

#if defined(CHALET_WIN32)
		bool result = ::ReadFile(pipe.m_read, static_cast<LPVOID>(inBuffer.data()), static_cast<DWORD>(bufferSize), static_cast<LPDWORD>(&bytesRead), nullptr) == TRUE;
		if (!result)
			bytesRead = 0;
#else
		bytesRead = ::read(pipe.m_read, inBuffer.data(), bufferSize);
#endif
		if (bytesRead > 0)
		{
			onRead(std::string(inBuffer.data(), bytesRead));
		}
		else
			break;
	}
}
}
