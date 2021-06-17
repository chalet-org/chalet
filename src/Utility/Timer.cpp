/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Utility/Timer.hpp"

namespace chalet
{
/*****************************************************************************/
Timer::Timer()
{
	m_start = clock::now();
}

/*****************************************************************************/
void Timer::restart()
{
	m_start = clock::now();
}

/*****************************************************************************/
std::int64_t Timer::stop()
{
	m_end = clock::now();

	auto executionTime = m_end - m_start;

	return std::chrono::duration_cast<std::chrono::milliseconds>(executionTime).count();
}

/*****************************************************************************/
std::string Timer::asString()
{
	auto end = clock::now();
	auto executionTime = end - m_start;

	auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(executionTime).count();

	if (milliseconds == 0)
		return std::string();

	return fmt::format("{}ms", milliseconds);
}
}