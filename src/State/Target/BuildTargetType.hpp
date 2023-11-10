/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

namespace chalet
{
enum class BuildTargetType
{
	Unknown,
	Source,
	SubChalet,
	CMake,
	Script,
	Process,
	Validation,
};
}
