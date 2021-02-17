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
	sp::ssize_t bytesRead = 1;

	if (inOptions.onStdout != nullptr)
	{
		while (bytesRead > 0)
		{
			bytesRead = sp::pipe_read(process.cout, buffer.data(), buffer.size());
			if (bytesRead <= 0)
				break;

			inOptions.onStdout(std::string(buffer.data(), bytesRead));

			for (auto& c : buffer)
				c = 0;
		}
	}

	if (inOptions.onStderr != nullptr)
	{
		bytesRead = 1;
		while (bytesRead > 0)
		{
			bytesRead = sp::pipe_read(process.cerr, buffer.data(), buffer.size());
			if (bytesRead <= 0)
				break;

			inOptions.onStderr(std::string(buffer.data(), bytesRead));

			for (auto& c : buffer)
				c = 0;
		}
	}

	// Note: On Windows, the underlying call to WaitForSingleObject takes the most time
	process.close();

	return process.returncode;
}
}
