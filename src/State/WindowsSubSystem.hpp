/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_WINDOWS_SUBSYSTEM_HPP
#define CHALET_WINDOWS_SUBSYSTEM_HPP

namespace chalet
{
enum class WindowsSubSystem : ushort
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

#endif // CHALET_WINDOWS_SUBSYSTEM_HPP
