/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Terminal/Spinner.hpp"

#include <signal.h>

#include "Terminal/Environment.hpp"
#include "Terminal/Output.hpp"
#include "Utility/SignalHandler.hpp"

namespace chalet
{
namespace
{
static struct
{
	std::mutex mutex;
	Spinner* spinner = nullptr;
} state;

/*****************************************************************************/
void signalHandler(int inSignal)
{
	if (state.spinner)
	{
		state.spinner->stop();
	}

	std::lock_guard<std::mutex> lock(state.mutex);

	UNUSED(inSignal);
	std::string output{ "\b\b  \b\b" };
	output += Output::getAnsiStyle(Output::theme().reset);
	std::cout.write(output.data(), output.size());
}
}

/*****************************************************************************/
Spinner::~Spinner()
{
	SignalHandler::remove(SIGINT, signalHandler);
	SignalHandler::remove(SIGTERM, signalHandler);
	SignalHandler::remove(SIGABRT, signalHandler);

	destroy();
}

/*****************************************************************************/
void Spinner::start()
{
	SignalHandler::add(SIGINT, signalHandler);
	SignalHandler::add(SIGTERM, signalHandler);
	SignalHandler::add(SIGABRT, signalHandler);

	destroy();

	m_thread = std::make_unique<std::thread>(&Spinner::doRegularEllipsis, this);
	state.spinner = this;
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
	state.spinner = nullptr;

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
		std::lock_guard<std::mutex> lock(state.mutex);
		std::string output{ " ... " };
		std::cout.write(output.data(), output.size());
		std::cout.flush();
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
			std::lock_guard<std::mutex> lock(state.mutex);
			std::cout.write(output.data(), output.size());
			std::cout.flush();
		}

		if (!sleepWithContext(frameTime))
			break;

		++i;

		if (i == 4)
			i = 0;
	}

	std::string output{ "\b\b\b\b\b ... " };
	std::cout.write(output.data(), output.size());
	std::cout.flush();
}

}
