/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include "Platform/Arch.hpp"
#include "Utility/Version.hpp"

namespace chalet
{
struct WindowsManifestGenSettings
{
	std::string name;
	Version version;
	Arch::Cpu cpu = Arch::Cpu::Unknown;

	bool disableWindowFiltering = false;
	bool dpiAwareness = false;
	bool longPathAware = false;
	bool disableGdiScaling = false;
	bool unicode = false;
	bool segmentHeap = false;
	bool compatibility = false;
};
}
