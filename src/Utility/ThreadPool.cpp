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

#include "Utility/ThreadPool.hpp"

namespace chalet
{
/*****************************************************************************/
ThreadPool::ThreadPool(const std::size_t inThreads)
{
	for (std::size_t i = 0; i < inThreads; ++i)
	{
		m_workers.emplace_back(&ThreadPool::workerThread, this);
	}
}

/*****************************************************************************/
ThreadPool::~ThreadPool()
{
	stop();

	m_condition.notify_all();
	for (auto& worker : m_workers)
		worker.join();
}

/*****************************************************************************/
void ThreadPool::stop()
{
	std::unique_lock<std::mutex> lock(m_queueMutex);
	m_stop = true;

	while (!m_tasks.empty())
		m_tasks.pop();
}

/*****************************************************************************/
void ThreadPool::workerThread()
{
	auto waitCondition = [this]() {
		return this->m_stop || !this->m_tasks.empty();
	};

	while (true)
	{
		std::function<void()> task;

		{
			std::unique_lock<std::mutex> lock(m_queueMutex);
			m_condition.wait(lock, waitCondition);

			if (m_stop && m_tasks.empty())
				return;

			task = std::move(m_tasks.front());
			m_tasks.pop();
		}

		task();
	}
}
}
