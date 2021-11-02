/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_COMPILER_INFO_HPP
#define CHALET_COMPILER_INFO_HPP

namespace chalet
{
struct CompilerInfo
{
	std::string path;
	std::string description;
	std::string version;
	std::string arch;
	std::string binDir;
	std::string libDir;
	std::string includeDir;

	uint versionMajorMinor;
	uint versionPatch;
};
}

#endif // CHALET_COMPILER_INFO_HPP
