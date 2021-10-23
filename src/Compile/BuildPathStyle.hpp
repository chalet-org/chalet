/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_BUILD_PATH_STYLE_HPP
#define CHALET_BUILD_PATH_STYLE_HPP

namespace chalet
{
enum class BuildPathStyle : ushort
{
	None,
	Configuration,
	ArchConfiguration,
	TargetTriple,
	ToolchainName,
};
}

#endif // CHALET_BUILD_PATH_STYLE_HPP
