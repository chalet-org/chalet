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

#include <memory>

namespace chalet
{
/*****************************************************************************/
template <class T, class... Args>
std::future<typename std::invoke_result_t<T, Args...>> ThreadPool::enqueue(T&& f, Args&&... args)
{
	using return_type = typename std::invoke_result_t<T, Args...>;

	auto task = std::make_shared<std::packaged_task<return_type()>>(
		std::bind(std::forward<T>(f), std::forward<Args>(args)...));

	std::future<return_type> res = task->get_future();

	{
		std::unique_lock<std::mutex> lock(m_queueMutex);

		if (m_stop)
			CHALET_THROW(std::runtime_error("enqueue on stopped ThreadPool"));

		m_tasks.emplace([task]() {
			(*task)();
		});
	}

	m_condition.notify_one();

	return res;
}
}
