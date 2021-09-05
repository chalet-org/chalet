/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Process/RunningProcess.hpp"

namespace chalet
{
/*****************************************************************************/
template <class T, size_t Size>
void RunningProcess::read(const PipeHandle inFileNo, std::array<T, Size>& inBuffer, const std::uint8_t inBufferSize, const ProcessOptions::PipeFunc& onRead)
{
	if (onRead != nullptr)
	{
		auto& pipe = getFilePipe(inFileNo);
		ssize_t bytesRead = 0;
		std::size_t bufferSize = inBufferSize > 0 ? static_cast<std::size_t>(inBufferSize) : inBuffer.size();
		while (true)
		{
			if (m_killed)
			{
				std::cout << std::endl;
				break;
			}

			bytesRead = ::read(pipe.m_read, inBuffer.data(), bufferSize);
			if (bytesRead > 0)
				onRead(std::string(inBuffer.data(), bytesRead));
			else
				break;
		}
	}
}
}
