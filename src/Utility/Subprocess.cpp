/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Utility/Subprocess.hpp"

#include <array>

#include "Libraries/SubprocessApi.hpp"

namespace chalet
{
/*****************************************************************************/
// TODO: Handle terminate/interrupt signals!
int Subprocess::run(const StringList& inCmd, SubprocessOptions&& inOptions)
{
	auto process = sp::RunBuilder(const_cast<StringList&>(inCmd))
					   .cerr(static_cast<sp::PipeOption>(inOptions.stderrOption))
					   .cout(static_cast<sp::PipeOption>(inOptions.stdoutOption))
					   .env(std::move(inOptions.env))
					   .cwd(std::move(inOptions.cwd))
					   .popen();

	if (inOptions.onStdOut != nullptr || inOptions.onStdErr != nullptr)
	{
		std::array<char, 128> buffer{ 0 };
		sp::ssize_t bytesRead = 1;

		if (inOptions.onStdOut != nullptr)
		{
			while (bytesRead > 0)
			{
				bytesRead = sp::pipe_read(process.cout, buffer.data(), buffer.size());
				if (bytesRead > 0)
				{
					inOptions.onStdOut(std::string(buffer.data(), bytesRead));
					memset(buffer.data(), 0, sizeof(char) * buffer.size());
				}
			}
		}

		if (inOptions.onStdErr != nullptr)
		{
			bytesRead = 1;
			while (bytesRead > 0)
			{
				bytesRead = sp::pipe_read(process.cerr, buffer.data(), buffer.size());
				if (bytesRead > 0)
				{
					inOptions.onStdErr(std::string(buffer.data(), bytesRead));
					memset(buffer.data(), 0, sizeof(char) * buffer.size());
				}
			}
		}
	}

	// Note: On Windows, the underlying call to WaitForSingleObject takes the most time
	process.close();

	// std::cout << "Exit code of last subprocess: " << process.returncode << std::endl;

	return process.returncode;
}
}
