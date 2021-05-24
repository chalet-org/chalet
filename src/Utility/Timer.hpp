/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_TIMER_HPP
#define CHALET_TIMER_HPP

#include <chrono>

namespace chalet
{
class Timer
{
	using clock = std::chrono::steady_clock;

public:
	Timer();

	void restart();
	std::int64_t stop();
	std::string asString();

private:
	clock::time_point m_start;
	clock::time_point m_end;
};
}

#endif // CHALET_IMMEDIATE_TIMER_HPP
