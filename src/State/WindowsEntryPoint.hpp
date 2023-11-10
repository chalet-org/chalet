/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

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
