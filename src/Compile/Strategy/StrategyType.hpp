/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_COMPILE_STRATEGY_TYPE_HPP
#define CHALET_COMPILE_STRATEGY_TYPE_HPP

namespace chalet
{
enum class StrategyType : ushort
{
	None,
	Makefile,
	Ninja,
	Native,
	MSBuild,
};
}

#endif // CHALET_COMPILE_STRATEGY_TYPE_HPP
