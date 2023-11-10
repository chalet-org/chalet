/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

namespace chalet
{
enum class OptimizationLevel : ushort
{
	CompilerDefault,
	None,
	L1,
	L2,
	L3,
	Debug,
	Size,
	Fast
};
}
