/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_BUILD_TARGET_TYPE_HPP
#define CHALET_BUILD_TARGET_TYPE_HPP

namespace chalet
{
enum class BuildTargetType
{
	Unknown,
	Project,
	Script,
	SubChalet,
	CMake
};
}

#endif // CHALET_BUILD_TARGET_TYPE_HPP
