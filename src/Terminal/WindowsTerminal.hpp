/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_WINDOWS_TERMINAL_HPP
#define CHALET_WINDOWS_TERMINAL_HPP

namespace chalet
{
namespace WindowsTerminal
{
void reset();
void initialize();
void initializeCreateProcess();
void cleanup();
}
}

#endif // CHALET_WINDOWS_TERMINAL_HPP
