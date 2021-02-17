/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Utility/Subprocess.hpp"

#include "Libraries/SubprocessApi.hpp"

namespace chalet
{
/*****************************************************************************/
int Subprocess::run(const StringList& inCmd, const SubprocessOptions& inOptions)
{
	auto process = sp::RunBuilder(const_cast<StringList&>(inCmd))
					   .cerr(inOptions.onStderr == nullptr ? sp::PipeOption::close : sp::PipeOption::pipe)
					   .cout(inOptions.onStdout == nullptr ? sp::PipeOption::close : sp::PipeOption::pipe)
					   .cwd(inOptions.cwd)
					   .popen();

	std::array<char, 128> buffer{ 0 };
	sp::ssize_t stdoutBytesRead = 1;
	sp::ssize_t stderrBytesRead = 1;

	while (stdoutBytesRead > 0 || stderrBytesRead > 0)
	{
		if (inOptions.onStdout != nullptr)
		{
			stdoutBytesRead = sp::pipe_read(process.cout, buffer.data(), buffer.size());
			if (stdoutBytesRead > 0)
			{
				inOptions.onStdout(std::string(buffer.data(), stdoutBytesRead));

				for (auto& c : buffer)
					c = 0;
			}
		}

		if (inOptions.onStderr != nullptr)
		{
			stderrBytesRead = sp::pipe_read(process.cerr, buffer.data(), buffer.size());
			if (stderrBytesRead <= 0)
				break;

			inOptions.onStderr(std::string(buffer.data(), stderrBytesRead));

			for (auto& c : buffer)
				c = 0;
		}
	}

	// Note: On Windows, the underlying call to WaitForSingleObject takes the most time
	process.close();

	return process.returncode;
}
}
