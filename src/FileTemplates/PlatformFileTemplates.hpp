/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_PLATFORM_FILE_TEMPLATES_HPP
#define CHALET_PLATFORM_FILE_TEMPLATES_HPP

#include "Core/Arch.hpp"

namespace chalet
{
struct MacosDiskImageTarget;

namespace PlatformFileTemplates
{
std::string linuxDesktopEntry();
std::string macosInfoPlist();
std::string minimumWindowsAppManifest();
std::string minimumWindowsAppManifestWithCompatibility();
std::string generalWindowsAppManifest(const std::string& inName, const Arch::Cpu inCpu);
std::string loadedWindowsAppManifest(const std::string& inName, const Arch::Cpu inCpu);
std::string windowsManifestResource(const std::string& inManifestFile, const bool inDllPrivateDeps = false);
std::string windowsIconResource(const std::string& inIconFile);
}
}

#endif // CHALET_PLATFORM_FILE_TEMPLATES_HPP
