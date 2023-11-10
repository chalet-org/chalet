/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

namespace chalet
{
enum class WindowsSubSystem : u16
{
	None,
	BootApplication,
	Console,
	Windows,
	Native,
	// NativeWDM,
	Posix,
	EfiApplication,
	EfiBootServiceDriver,
	EfiRom,
	EfiRuntimeDriver,
};
}
