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
constexpr std::string_view linuxDesktopEntry();
constexpr std::string_view macosInfoPlist();
constexpr std::string_view macosDmgApplescript();
constexpr std::string_view windowsAppManifest();
}
}

#include "Builder/PlatformFile.inl"

#endif // CHALET_PLATFORM_FILE_HPP
