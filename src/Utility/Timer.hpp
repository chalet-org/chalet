/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include <chrono>

namespace chalet
{
class Timer
{
	using clock = std::chrono::steady_clock;

public:
	Timer();

	void restart();
	i64 stop();
	std::string asString(const bool inRestart = false);

private:
	clock::time_point m_start;
	clock::time_point m_end;
};
}
