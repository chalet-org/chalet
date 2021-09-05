/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Utility/Subprocess.hpp"

#include <array>
#include <atomic>

#include "Libraries/SubprocessApi.hpp"
#include "Terminal/OSTerminal.hpp"
#include "Utility/Subprocess2.hpp"

#ifdef CHALET_MSVC
	#pragma warning(push)
	#pragma warning(disable : 4244)
#endif

namespace chalet
{
namespace
{
std::vector<sp::Popen*> s_procesess;
std::atomic<int> s_lastErrorCode = 0;
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

	if (s_procesess.empty())
		OSTerminal::reset();
}

/*****************************************************************************/
void subProcessSignalHandler(int inSignal)
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

	OSTerminal::reset();
}
}

/*****************************************************************************/
int Subprocess::getLastExitCode()
{
	return s_lastErrorCode;
}

/*****************************************************************************/
int Subprocess::run(const StringList& inCmd, SubprocessOptions&& inOptions)
{
	CHALET_TRY
	{
		auto process = sp::RunBuilder(const_cast<StringList&>(inCmd))
						   .cerr(static_cast<sp::PipeOption>(inOptions.stderrOption))
						   .cout(static_cast<sp::PipeOption>(inOptions.stdoutOption))
						   .cwd(std::move(inOptions.cwd))
						   .popen();

		s_procesess.push_back(&process);

		if (inOptions.onCreate != nullptr)
		{
			inOptions.onCreate(static_cast<int>(process.pid));
		}

		if (!s_initialized)
		{
			::signal(SIGINT, subProcessSignalHandler);
			::signal(SIGTERM, subProcessSignalHandler);
			::signal(SIGABRT, subProcessSignalHandler);
			s_initialized = true;
		}

		static auto onDefaultStdErr = [](std::string inData) {
			std::cerr << inData << std::flush;
		};

		if (inOptions.stderrOption == PipeOption::StdErr)
		{
			inOptions.onStdErr = onDefaultStdErr;
		}

		if (inOptions.onStdOut != nullptr || inOptions.onStdErr != nullptr)
		{
			static std::array<char, 256> buffer{ 0 };
			sp::ssize_t bytesRead = 0;

			if (inOptions.onStdOut != nullptr)
			{
				do
				{
					bytesRead = sp::pipe_read(process.cout, buffer.data(), buffer.size());
					if (bytesRead > 0)
					{
						inOptions.onStdOut(std::string(buffer.data(), bytesRead));
						// std::fill_n(buffer.data(), bytesRead, 0);
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
						// std::fill_n(buffer.data(), bytesRead, 0);
					}
				} while (bytesRead > 0);
			}
		}

		removeProcess(process);

		// Note: On Windows, the underlying call to WaitForSingleObject takes the most time
		process.close();

		// std::cout << "Exit code of last subprocess: " << process.returncode << std::endl;
		s_lastErrorCode = process.returncode;

		return process.returncode;
	}
	CHALET_CATCH(const std::exception& err)
	{
		CHALET_EXCEPT_ERROR("subprocess error: {}", err.what());
		return -1;
	}
}

/*****************************************************************************/
void Subprocess::haltAllProcesses(const int inSignal)
{
	subProcessSignalHandler(inSignal);
}
}

#ifdef CHALET_MSVC
	#pragma warning(pop)
#endif
