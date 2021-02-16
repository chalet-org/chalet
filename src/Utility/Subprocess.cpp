/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Utility/Subprocess.hpp"

#include "Libraries/Format.hpp"
#include "Libraries/SubprocessApi.hpp"
#include "Terminal/Output.hpp"
#include "Utility/Timer.hpp"

namespace chalet
{
/*****************************************************************************/
int Subprocess::run(StringList inCmd, const PipeFunc& onStdout)
{
	auto process = sp::RunBuilder(inCmd)
					   .cerr(sp::PipeOption::cout)
					   .cout(sp::PipeOption::pipe)
					   .popen();

	std::array<char, 256> buffer{ 0 };
	sp::ssize_t bytesRead = 1;

	if (onStdout != nullptr)
	{
		while (bytesRead > 0)
		{
			bytesRead = sp::pipe_read(process.cout, buffer.data(), buffer.size());
			if (bytesRead <= 0)
				break;

			onStdout(std::string(buffer.data(), bytesRead));

			for (auto& c : buffer)
				c = 0;
		}
	}

	// this can take anywhere from 0ms to 20ms on windows
	process.closePipes();

	Timer timer;

	if (process.pid)
		process.waitTest();

	auto result = timer.stop();
	Output::print(Color::Reset, fmt::format("sp::Popen::closeProcessWait() time: {}ms", result));
	timer.restart();

	process.closeProcess();

	result = timer.stop();
	Output::print(Color::Reset, fmt::format("sp::Popen::closeProcess() time: {}ms", result));

	process.closeCleanup();

	return process.returncode;
}
}
