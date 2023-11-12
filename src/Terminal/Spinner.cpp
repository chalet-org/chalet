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
struct
{
	std::mutex mutex;
	Spinner* spinner = nullptr;
	size_t refCount = 0;
} state;

/*****************************************************************************/
void signalHandler(i32 inSignal)
{
	std::lock_guard<std::mutex> lock(state.mutex);
	if (state.spinner)
	{
		if (state.spinner->cancel())
		{
			UNUSED(inSignal);
			std::string output = Output::getAnsiStyle(Output::theme().reset);
			std::cout.write(output.data(), output.size());

			// std::exit(1);
		}
	}
}
}

/*****************************************************************************/
Spinner::Spinner()
{
	if (state.refCount == 0)
	{
		SignalHandler::add(SIGINT, signalHandler);
		SignalHandler::add(SIGTERM, signalHandler);
		SignalHandler::add(SIGABRT, signalHandler);
	}

	if (state.refCount < std::numeric_limits<size_t>::max())
		state.refCount++;
}

/*****************************************************************************/
Spinner::~Spinner()
{
	stop();

	if (state.refCount > 0)
		state.refCount--;

	if (state.refCount == 0)
	{
		SignalHandler::remove(SIGINT, signalHandler);
		SignalHandler::remove(SIGTERM, signalHandler);
		SignalHandler::remove(SIGABRT, signalHandler);
	}
}

/*****************************************************************************/
bool Spinner::start()
{
	if (stop())
	{
		m_thread = std::make_unique<std::thread>(&Spinner::doRegularEllipsis, this);
		state.spinner = this;
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
	bool result = false;

	if (m_thread != nullptr)
	{
		if (m_thread->joinable())
		{
			m_running = false;
			m_thread->join();
			m_thread.reset();
			result = true;
		}
	}
	else
	{
		result = true;
	}

	if (result && state.spinner == this)
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

	if (Shell::isContinuousIntegrationServer())
		return;

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

	if (!m_cancelled)
	{
		// std::lock_guard<std::mutex> lock(state.mutex);
		std::string output{ "\b\b\b\b\b ... " };
		std::cout.write(output.data(), output.size());
		std::cout.flush();
	}
}
}
