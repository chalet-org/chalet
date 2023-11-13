/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Terminal/Spinner.hpp"

#include <signal.h>

#include "System/SignalHandler.hpp"
#include "Terminal/Output.hpp"
#include "Terminal/Shell.hpp"

namespace chalet
{
namespace
{
Spinner* spinner = nullptr;

/*****************************************************************************/
void signalHandler(i32 inSignal)
{
	UNUSED(inSignal);

	if (spinner)
	{
		if (spinner->cancel())
		{
			auto output = Output::getAnsiStyle(Output::theme().reset);
			std::cout.write(output.data(), output.size());
		}
	}
}
}

/*****************************************************************************/
Spinner& Spinner::instance()
{
	if (spinner == nullptr)
	{
		spinner = new Spinner();

		SignalHandler::add(SIGINT, signalHandler);
		SignalHandler::add(SIGTERM, signalHandler);
		SignalHandler::add(SIGABRT, signalHandler);
	}

	return *spinner;
}

/*****************************************************************************/
bool Spinner::instanceCreated()
{
	return spinner != nullptr;
}

/*****************************************************************************/
bool Spinner::destroyInstance()
{
	if (spinner != nullptr)
	{
		SignalHandler::remove(SIGINT, signalHandler);
		SignalHandler::remove(SIGTERM, signalHandler);
		SignalHandler::remove(SIGABRT, signalHandler);

		delete spinner;
		spinner = nullptr;
		return true;
	}

	return false;
}

/*****************************************************************************/
Spinner::~Spinner()
{
	stop();
}

/*****************************************************************************/
bool Spinner::start()
{
	if (stop())
	{
		m_thread = std::make_unique<std::thread>(&Spinner::doRegularEllipsis, this);
		return true;
	}

	return false;
}

/*****************************************************************************/
bool Spinner::cancel()
{
	m_cancelled = true;
	return stop();
}

/*****************************************************************************/
bool Spinner::stop()
{
	bool result = true;
	if (m_thread != nullptr)
	{
		result = false;
		if (m_thread->joinable())
		{
			m_running = false;
			m_thread->join();
			m_thread.reset();
			result = true;
		}
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
		std::string output{ " ... " };
		std::cout.write(output.data(), output.size());
		std::cout.flush();

		if (Shell::isContinuousIntegrationServer())
			return;
	}

	constexpr auto frameTime = std::chrono::milliseconds(250);

	// first "frame" - keep output minimal
	if (!sleepWithContext(frameTime))
		return;

	m_running = true;

	u32 i = 0;
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
			std::cout.write(output.data(), output.size());
			std::cout.flush();
		}

		if (!sleepWithContext(frameTime))
			break;

		++i;

		if (i == 4)
			i = 0;
	}

	if (!m_cancelled)
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		std::string output{ "\b\b\b\b\b ... " };
		std::cout.write(output.data(), output.size());
		std::cout.flush();
	}
}
}
