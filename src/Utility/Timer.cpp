/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Utility/Timer.hpp"

#define CHALET_SHOW_MICROSECONDS 0
#define CHALET_SHOW_NANOSECONDS 0
#define CHALET_SHOW_PICOSECONDS 0

namespace chalet
{
#if CHALET_SHOW_PICOSECONDS
using std_chrono_picoseconds = std::chrono::duration<long long, std::pico>;
#endif

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
i64 Timer::stop()
{
	m_end = clock::now();
	auto executionTime = m_end - m_start;

	return std::chrono::duration_cast<std::chrono::milliseconds>(executionTime).count();
}

/*****************************************************************************/
std::string Timer::asString(const bool inRestart)
{
	auto end = clock::now();
	auto executionTime = end - m_start;

	if (inRestart)
		m_start = clock::now();

	u32 milliseconds = static_cast<u32>(std::chrono::duration_cast<std::chrono::milliseconds>(executionTime).count());
	if (milliseconds == 0)
	{
#if CHALET_SHOW_MICROSECONDS
		u64 microseconds = static_cast<u64>(std::chrono::duration_cast<std::chrono::microseconds>(executionTime).count());
		if (microseconds == 0)
		{
	#if CHALET_SHOW_NANOSECONDS
			u64 nanoseconds = static_cast<u64>(std::chrono::duration_cast<std::chrono::nanoseconds>(executionTime).count());
			if (nanoseconds == 0)
			{
		#if CHALET_SHOW_PICOSECONDS
				u64 picoseconds = static_cast<u64>(std::chrono::duration_cast<std_chrono_picoseconds>(executionTime).count());
				auto result = std::to_string(picoseconds);
				return fmt::format("{}ps", result.substr(0, 5));
		#else
				return std::string();
		#endif
			}
			else
			{
				auto result = std::to_string(nanoseconds);
				return fmt::format("{}ns", result.substr(0, 5));
			}
	#else
			return std::string();
	#endif
		}
		else
		{
			auto result = std::to_string(microseconds);
			return fmt::format("{}µs", result.substr(0, 5));
		}
#else
		return std::string();
#endif
	}

	if (milliseconds < 1000)
		return fmt::format("{}ms", milliseconds);

	u32 seconds = milliseconds / 1000;
	milliseconds %= 1000;
	auto ms = fmt::format("{:0>3}", milliseconds);

	if (seconds < 60)
		return fmt::format("{}.{}s", seconds, ms);

	u32 minutes = seconds / 60;
	seconds %= 60;
	auto sec = fmt::format("{:0>2}", seconds);

	if (minutes < 60)
		return fmt::format("{}:{}.{}m", minutes, sec, ms);

	u32 hours = minutes / 60;
	minutes %= 60;
	auto min = fmt::format("{:0>2}", minutes);
	return fmt::format("{}:{}:{}.{}h", hours, min, sec, ms);
}
}