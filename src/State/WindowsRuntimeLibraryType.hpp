/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_WINDOWS_RUNTIME_LIBRARY_TYPE_HPP
#define CHALET_WINDOWS_RUNTIME_LIBRARY_TYPE_HPP

namespace chalet
{
enum class WindowsRuntimeLibraryType : ushort
{
	MultiThreaded,
	MultiThreadedDebug,
	MultiThreadedDLL,
	MultiThreadedDebugDLL,
};
}

#endif // CHALET_WINDOWS_RUNTIME_LIBRARY_TYPE_HPP
