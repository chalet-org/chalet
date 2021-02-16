/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Utility/Subprocess.hpp"

#include "Libraries/SubprocessApi.hpp"

namespace chalet
{
/*****************************************************************************/
int Subprocess::run(StringList inCmd, const PipeFunc& onStdout)
{
	auto process = sp::RunBuilder(std::move(inCmd))
					   .cerr(sp::PipeOption::cout)
					   .cout(sp::PipeOption::pipe)
					   .popen();

	std::array<char, 256> buffer{ 0 };
	sp::ssize_t bytesRead = 1;

	if (onStdout != nullptr)
	{
		while (bytesRead > 0)
		{
			bytesRead = subprocess::pipe_read(process.cout, buffer.data(), buffer.size());
			if (bytesRead <= 0)
				break;

			onStdout(std::string(buffer.data(), bytesRead));

			for (auto& c : buffer)
				c = 0;
		}
	}

	int returnCode = process.wait();
	process.close();
	return returnCode;
}
}
