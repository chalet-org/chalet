/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_SPINNER_HPP
#define CHALET_SPINNER_HPP

#include <atomic>
#include <thread>

namespace chalet
{
struct Spinner
{
	Spinner();
	CHALET_DISALLOW_COPY_MOVE(Spinner);
	~Spinner();

	void start();
	void stop();

private:
	void destroy();

	void doRegularEllipsis();

	std::atomic<bool> m_running = false;
	std::unique_ptr<std::thread> m_thread;
};
}

#endif // CHALET_SPINNER_HPP
