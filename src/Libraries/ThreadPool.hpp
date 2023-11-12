/*
	Copyright (c) 2012 Jakob Progsch, Václav Zeman

	This software is provided 'as-is', without any express or implied
	warranty. In no event will the authors be held liable for any damages
	arising from the use of this software.

	Permission is granted to anyone to use this software for any purpose,
	including commercial applications, and to alter it and redistribute it
	freely, subject to the following restrictions:

	1. The origin of this software must not be misrepresented; you must not
	claim that you wrote the original software. If you use this software
	in a product, an acknowledgment in the product documentation would be
	appreciated but is not required.

	2. Altered source versions must be plainly marked as such, and must not be
	misrepresented as being the original software.

	3. This notice may not be removed or altered from any source
	distribution.
*/
/*
	With modifications for use with Chalet
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include <atomic>
#include <condition_variable>
#include <future>
#include <mutex>
#include <queue>
#include <thread>

namespace chalet
{
class ThreadPool
{
public:
	explicit ThreadPool(const size_t inThreads);
	CHALET_DISALLOW_COPY_MOVE(ThreadPool);
	~ThreadPool();

	template <class T, class... Args>
	std::future<typename std::invoke_result_t<T, Args...>> enqueue(T&& f, Args&&... args);

	void stop();
	size_t threads() const noexcept;

private:
	void workerThread();

	size_t m_threads = 0;

	std::vector<std::thread> m_workers;
	std::queue<std::function<void()>> m_tasks;

	std::mutex m_queueMutex;
	std::condition_variable m_condition;

	std::atomic_bool m_stopped = false;
};
}

#include "Libraries/ThreadPool.inl"
