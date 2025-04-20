/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include "FileTemplates/WindowsManifestGenSettings.hpp"
#include "Platform/Arch.hpp"

namespace chalet
{
namespace PlatformFileTemplates
{
std::string linuxDesktopEntry();
std::string macosInfoPlist();
std::string windowsAppManifest(const WindowsManifestGenSettings& inSettings);
std::string windowsManifestResource(const std::string& inManifestFile, const bool inDllPrivateDeps = false);
std::string windowsIconResource(const std::string& inIconFile);
}
}
