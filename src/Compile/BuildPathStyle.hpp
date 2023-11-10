/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

namespace chalet
{
enum class BuildPathStyle : u16
{
	None,
	Configuration,
	ArchConfiguration,
	TargetTriple,
	ToolchainName,
};
}
