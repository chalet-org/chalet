/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Utility/Subprocess.hpp"

#include "Libraries/SubprocessApi.hpp"

namespace chalet
{
/*****************************************************************************/
int Subprocess::run(const StringList& inCmd, const PipeFunc& onStdout)
{
	auto popen = sp::RunBuilder(inCmd).cerr(sp::PipeOption::pipe).cout(sp::PipeOption::pipe).popen();
	std::array<char, 256> buffer_cout = { 0 };
	bool has_cout = true;

	if (onStdout != nullptr)
	{
		while (has_cout)
		{
			has_cout = subprocess::pipe_read(popen.cout, buffer_cout.data(), buffer_cout.size()) != 0;
			if (!has_cout)
				break;

			onStdout(buffer_cout.data());
			std::fill(buffer_cout.begin(), buffer_cout.end(), 0);
		}
	}

	popen.close();

	return popen.returncode == 0;
}
}
