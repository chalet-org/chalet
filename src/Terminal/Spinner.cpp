/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Terminal/Spinner.hpp"

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
Spinner::Spinner()
{
	start();
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

	m_thread = std::make_unique<std::thread>(&Spinner::doRegularEllipsis, this);
}

/*****************************************************************************/
void Spinner::stop()
{
	destroy();
}

/*****************************************************************************/
void Spinner::destroy()
{
	if (m_thread != nullptr)
	{
		m_running = false;
		m_thread->join();

		m_thread.reset();
		m_thread = nullptr;
	}
}

/*****************************************************************************/
void Spinner::doRegularEllipsis()
{
	m_running = true;
	uint i = 0;
	while (true)
	{
		std::string output;
		switch (i % 4)
		{
			case 0: output = "\b\b\b\b\b ... "; break;
			case 1: output = "\b\b\b\b\b     "; break;
			case 2: output = "\b\b\b\b\b .   "; break;
			case 3: output = "\b\b\b\b\b ..  "; break;
			default: break;
		}

		std::cout << output << std::flush;
		std::this_thread::sleep_for(std::chrono::milliseconds(333));
		++i;
		if (i == 4)
			i = 0;

		if (!m_running)
			break;
	}

	std::cout << "\b\b\b\b\b ... " << std::flush;
}

}
