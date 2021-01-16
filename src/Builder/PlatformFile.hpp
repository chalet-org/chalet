/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_PLATFORM_FILE_HPP
#define CHALET_PLATFORM_FILE_HPP

namespace chalet
{
namespace PlatformFile
{
constexpr Constant linuxDesktopEntry();
constexpr Constant macosInfoPlist();
constexpr Constant macosDmgApplescript();
constexpr Constant windowsAppManifest();
}
}

#include "Builder/PlatformFile.inl"

#endif // CHALET_PLATFORM_FILE_HPP
