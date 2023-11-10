/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

namespace chalet
{
struct CompilerInfo
{
	std::string path;
	std::string description;
	std::string version;
	// std::string arch;
	std::string binDir;
	std::string libDir;
	std::string includeDir;

	u32 versionMajorMinor;
	u32 versionPatch;
};
}
