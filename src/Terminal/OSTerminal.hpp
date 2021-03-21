/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_OS_TERMINAL_HPP
#define CHALET_OS_TERMINAL_HPP

namespace chalet
{
class OSTerminal
{
	friend class Application;

public:
	static void reset();

private:
	OSTerminal() = default;

	void initialize();
	void cleanup();

#if defined(CHALET_WIN32)
	uint m_consoleCp = 0;
	uint m_consoleOutputCp = 0;
#endif

	bool m_initialized = false;
};
}

#endif // CHALET_OS_TERMINAL_HPP
