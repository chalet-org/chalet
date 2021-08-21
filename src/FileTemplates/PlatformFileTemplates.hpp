/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_PLATFORM_FILE_TEMPLATES_HPP
#define CHALET_PLATFORM_FILE_TEMPLATES_HPP

namespace chalet
{
namespace PlatformFileTemplates
{
std::string linuxDesktopEntry();
std::string macosInfoPlist();
std::string macosDmgApplescript(const std::string& inAppName, const bool inHasBackground);
std::string minimumWindowsAppManifest();
std::string generalWindowsAppManifest(const std::string& inName = "${name}", const std::string& inDescription = "${description}", const std::string& inVersion = "${version}");
std::string loadedWindowsAppManifest(const std::string& inName = "${name}", const std::string& inDescription = "${description}", const std::string& inVersion = "${version}");
std::string windowsManifestResource(const std::string& inManifestFile, const bool inDllPrivateDeps = false);
std::string windowsIconResource(const std::string& inIconFile);
}
}

#endif // CHALET_PLATFORM_FILE_TEMPLATES_HPP
