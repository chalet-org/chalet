/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Terminal/Spinner.hpp"

#include <signal.h>

#include "Terminal/Environment.hpp"

namespace chalet
{
namespace
{
/*****************************************************************************/
void signalHandler(int inSignal)
{
	UNUSED(inSignal);
	std::cout << "\b\b  \b\b" << std::flush;
}
}

/*****************************************************************************/
Spinner::~Spinner()
{
	destroy();
}

/*****************************************************************************/
void Spinner::start()
{
	::signal(SIGINT, signalHandler);
	::signal(SIGTERM, signalHandler);
	::signal(SIGABRT, signalHandler);

	destroy();

	m_thread = std::make_unique<std::thread>(&Spinner::doRegularEllipsis, this);
}

/*****************************************************************************/
bool Spinner::stop()
{
	return destroy();
}

/*****************************************************************************/
bool Spinner::destroy()
{
	bool result = m_running;

	if (m_thread != nullptr)
	{
		m_running = false;
		m_thread->join();
		m_thread.reset();
	}

	return result;
}

/*****************************************************************************/
bool Spinner::sleepWithContext(const std::chrono::milliseconds& inLength)
{
	auto start = clock::now();
	std::chrono::milliseconds ms{ 0 };
	auto step = std::chrono::milliseconds(1);
	while (ms < inLength)
	{
		auto finish = clock::now();
		ms = std::chrono::duration_cast<std::chrono::milliseconds>(finish - start);

		if (!m_running)
			return false;

		std::this_thread::sleep_for(step);
	}

	return true;
}

/*****************************************************************************/
void Spinner::doRegularEllipsis()
{
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		std::cout << " ... " << std::flush;
	}

	if (Environment::isContinuousIntegrationServer())
		return;

	constexpr auto frameTime = std::chrono::milliseconds(333);

	// first "frame" - keep output minimal
	if (!sleepWithContext(frameTime))
		return;

	m_running = true;

	uint i = 0;
	while (m_running)
	{
		std::string output;
		switch (i % 4)
		{
			case 0: output = "\b\b\b\b\b     "; break;
			case 1: output = "\b\b\b\b\b .   "; break;
			case 2: output = "\b\b\b\b\b ..  "; break;
			case 3: output = "\b\b\b\b\b ... "; break;
			default: break;
		}

		{
			std::lock_guard<std::mutex> lock(m_mutex);
			std::cout << output << std::flush;
		}

		if (!sleepWithContext(frameTime))
			break;

		++i;

		if (i == 4)
			i = 0;
	}

	std::cout << "\b\b\b\b\b ... " << std::flush;
}

}
