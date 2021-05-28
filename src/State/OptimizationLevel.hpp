/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_OPTIMIZATION_LEVEL_HPP
#define CHALET_OPTIMIZATION_LEVEL_HPP

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

#endif // CHALET_OPTIMIZATION_LEVEL_HPP
