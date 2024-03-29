/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

namespace chalet
{
enum class StrategyType : u16
{
	None,
	Makefile,
	Ninja,
	Native,
	MSBuild,
	XcodeBuild,
	Count,
};
}
