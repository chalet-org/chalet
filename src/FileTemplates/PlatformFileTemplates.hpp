/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include "Core/Arch.hpp"

namespace chalet
{
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
