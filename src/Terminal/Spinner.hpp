/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_SPINNER_HPP
#define CHALET_SPINNER_HPP

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

	static Spinner& instance();
	static bool instanceCreated();
	static bool destroyInstance();

	bool start();
	bool cancel();
	bool stop();

private:
	using clock = std::chrono::steady_clock;

	bool sleepWithContext(const std::chrono::milliseconds& inLength);

	void doRegularEllipsis();

	Unique<std::thread> m_thread;
	std::mutex m_mutex;

	std::atomic<bool> m_running = true;
	std::atomic<bool> m_cancelled = false;
};
}

#endif // CHALET_SPINNER_HPP
