/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Utility/Subprocess.hpp"

#include <array>

#include "Libraries/SubprocessApi.hpp"
#include "Terminal/OSTerminal.hpp"

namespace chalet
{
namespace
{
std::vector<sp::Popen*> s_procesess;
bool s_initialized = false;

/*****************************************************************************/
void removeProcess(const sp::Popen& inProcess)
{
	auto it = s_procesess.end();
	while (it != s_procesess.begin())
	{
		--it;
		sp::Popen* process = (*it);
		if (process->pid == inProcess.pid)
		{
			it = s_procesess.erase(it);
			return;
		}
	}
}

/*****************************************************************************/
void signalHandler(int inSignal)
{
	auto it = s_procesess.end();
	while (it != s_procesess.begin())
	{
		--it;
		sp::Popen* process = (*it);

		if (inSignal == SIGTERM)
			process->terminate();
		else
			process->send_signal(inSignal);

		it = s_procesess.erase(it);
	}
}
}

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

	s_procesess.push_back(&process);

	if (inOptions.onCreate != nullptr)
	{
		inOptions.onCreate(static_cast<int>(process.pid));
	}

	if (!s_initialized)
	{
		::signal(SIGINT, signalHandler);
		::signal(SIGTERM, signalHandler);
		::signal(SIGABRT, signalHandler);
		s_initialized = true;
	}

	if (inOptions.onStdOut != nullptr || inOptions.onStdErr != nullptr)
	{
		std::array<char, 128> buffer{ 0 };
		sp::ssize_t bytesRead = 0;

		if (inOptions.onStdOut != nullptr)
		{
			do
			{
				bytesRead = sp::pipe_read(process.cout, buffer.data(), buffer.size());
				if (bytesRead > 0)
				{
					inOptions.onStdOut(std::string(buffer.data(), bytesRead));
					memset(buffer.data(), 0, sizeof(char) * buffer.size());
				}
			} while (bytesRead > 0);
		}

		if (inOptions.onStdErr != nullptr)
		{
			do
			{
				bytesRead = sp::pipe_read(process.cerr, buffer.data(), buffer.size());
				if (bytesRead > 0)
				{
					inOptions.onStdErr(std::string(buffer.data(), bytesRead));
					memset(buffer.data(), 0, sizeof(char) * buffer.size());
				}
			} while (bytesRead > 0);
		}
	}

	removeProcess(process);

	// Note: On Windows, the underlying call to WaitForSingleObject takes the most time
	process.close();

	// std::cout << "Exit code of last subprocess: " << process.returncode << std::endl;

	OSTerminal::reset();

	return process.returncode;
}

/*****************************************************************************/
void Subprocess::haltAllProcesses(const int inSignal)
{
	signalHandler(inSignal);
}
}
