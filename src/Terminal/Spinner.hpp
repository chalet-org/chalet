/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include <atomic>
#include <mutex>
#include <thread>

namespace chalet
{
struct Spinner
{
	Spinner() = default;
	CHALET_DISALLOW_COPY_MOVE(Spinner);
	~Spinner();

	void start();
	bool cancel();
	bool stop();

private:
	using clock = std::chrono::steady_clock;

	bool sleepWithContext(const std::chrono::milliseconds& inLength);

	void doRegularEllipsis();

	std::atomic<bool> m_running = true;
	std::atomic<bool> m_cancelled = false;
	Unique<std::thread> m_thread;
};
}
