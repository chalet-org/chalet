/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_POSITION_INDEPENDENT_CODE_TYPE_HPP
#define CHALET_POSITION_INDEPENDENT_CODE_TYPE_HPP

namespace chalet
{
enum class PositionIndependentCodeType : ushort
{
	None,
	Auto,
	Shared,
	Executable,
};
}

#endif // CHALET_POSITION_INDEPENDENT_CODE_TYPE_HPP
