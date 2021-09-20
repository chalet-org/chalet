/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_WINDOWS_ENTRY_POINT_HPP
#define CHALET_WINDOWS_ENTRY_POINT_HPP

namespace chalet
{
enum class WindowsEntryPoint : ushort
{
	None,
	Main,
	MainUnicode,
	WinMain,
	WinMainUnicode,
	DllMain
};
}

#endif // CHALET_WINDOWS_ENTRY_POINT_HPP
