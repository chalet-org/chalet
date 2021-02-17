/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Utility/Subprocess.hpp"

namespace chalet
{
/*****************************************************************************/
int Subprocess::run(const StringList& inCmd, const SubprocessOptions& inOptions)
{
	auto process = sp::RunBuilder(const_cast<StringList&>(inCmd))
					   .cerr(inOptions.stderrOption)
					   .cout(inOptions.stderrOption)
					   .cwd(inOptions.cwd)
					   .check(true)
					   .popen();

	if (inOptions.onStdout != nullptr || inOptions.onStderr != nullptr)
	{
		std::array<char, 128> buffer{ 0 };
		sp::ssize_t bytesRead = 1;

		if (inOptions.onStdout != nullptr)
		{
			while (bytesRead > 0)
			{
				bytesRead = sp::pipe_read(process.cout, buffer.data(), buffer.size());
				if (bytesRead > 0)
				{
					inOptions.onStdout(std::string(buffer.data(), bytesRead));
					memset(buffer.data(), 0, sizeof(char) * buffer.size());
				}
			}
		}

		if (inOptions.onStderr != nullptr)
		{
			bytesRead = 1;
			while (bytesRead > 0)
			{
				bytesRead = sp::pipe_read(process.cerr, buffer.data(), buffer.size());
				if (bytesRead > 0)
				{
					inOptions.onStderr(std::string(buffer.data(), bytesRead));
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
