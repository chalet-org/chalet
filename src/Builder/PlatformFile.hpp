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
std::string linuxDesktopEntry();
std::string macosInfoPlist();
std::string macosDmgApplescript(const std::string& inAppName);
std::string windowsAppManifest();
}
}

#endif // CHALET_PLATFORM_FILE_HPP
